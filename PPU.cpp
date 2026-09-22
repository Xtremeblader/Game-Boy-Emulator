#include "PPU.h"
#include "MMU.h"
#include <algorithm>

PPU::PPU(MMU& mmu_ref) 
    : mmu(mmu_ref), 
      current_mode(OAM_SEARCH),
      cycles_in_mode(0),
      current_scanline(0),
      lcdc(0x91),
      stat(0x00),
      scy(0),
      scx(0),
      ly(0),
      lyc(0),
      bgp(0xFC),
      obp0(0xFF),
      obp1(0xFF),
      wy(0),
      wx(0),
      frame_ready(false),
      vblank_interrupt(false),
      lcd_stat_interrupt(false){
    framebuffer.fill(0x7FFF);  // White
}

void PPU::updateStatInterrupt(){
    bool signal = isPPUEnabled() && (
        (current_mode == HBLANK && getHBlankStatInterrupt()) ||
        (current_mode == VBLANK && getVBlankStatInterrupt()) ||
        (current_mode == OAM_SEARCH && getOAMInterrupt()) ||
        (ly == lyc && getLYCInterrupt()));
    if(signal && !stat_line) lcd_stat_interrupt = true;
    stat_line = signal;
}

void PPU::update(int cycles){
    if(!isPPUEnabled() || cycles <= 0) return;
    cycles_in_mode += cycles;
    while(true){
        int duration = current_mode == OAM_SEARCH ? 80 :
                       current_mode == PIXEL_TRANSFER ? 172 :
                       current_mode == HBLANK ? 204 : 456;
        if(cycles_in_mode < duration) break;
        cycles_in_mode -= duration;
        switch(current_mode){
            case OAM_SEARCH:
                current_mode = PIXEL_TRANSFER;
                break;
            case PIXEL_TRANSFER:
                renderScanline();
                current_mode = HBLANK;
                break;
            case HBLANK:
                ly = ++current_scanline;
                if(ly == 144){
                    current_mode = VBLANK;
                    vblank_interrupt = true;
                    frame_ready = true;
                } else current_mode = OAM_SEARCH;
                break;
            case VBLANK:
                ly = ++current_scanline;
                if(ly == 154){
                    ly = current_scanline = 0;
                    window_line = 0;
                    current_mode = OAM_SEARCH;
                }
                break;
        }
        updateStatInterrupt();
    }
}

void PPU::renderScanline(){
    if(current_scanline >= 144) return;
    background_colors.fill(0);
    auto* pixels = framebuffer.data() + current_scanline * 160;
    std::fill(pixels, pixels + 160, 0x7FFF);
    if(backgroundEnabled()){
        renderBackgroundScanline();
        renderWindowScanline();
    }
    if(spritesEnabled()) renderSpriteScanline();
}

uint16_t PPU::colorToRGB555(uint8_t pixel, uint8_t palette){
    // Extract 2-bit color from palette
    uint8_t palette_index = (palette >> (pixel * 2)) & 0x03;
    
    // Convert 2-bit grayscale to RGB555
    // 0 = white(31,31,31)
    // 1 = light gray(21,21,21)
    // 2 = dark gray(10,10,10)
    // 3 = black(0,0,0)
    uint8_t gray_value;
    switch(palette_index){
        case 0: gray_value = 31; break;  // White
        case 1: gray_value = 21; break;  // Light gray
        case 2: gray_value = 10; break;  // Dark gray
        case 3: gray_value = 0;  break;  // Black
        default: gray_value = 31; break;
    }
    
    // Pack into 555 RGB format(XBBBBBGGGGGRRRRR)
    return(gray_value << 10) | (gray_value << 5) | gray_value;
}

// I/O Register accessors
uint8_t PPU::readLCDC() const { return lcdc; }
void PPU::writeLCDC(uint8_t value){
    bool was_enabled = isPPUEnabled();
    lcdc = value;
    if(was_enabled != isPPUEnabled()){
        cycles_in_mode = 0;
        ly = current_scanline = 0;
        window_line = 0;
        current_mode = isPPUEnabled() ? OAM_SEARCH : HBLANK;
        frame_ready = vblank_interrupt = lcd_stat_interrupt = false;
        if(!isPPUEnabled()){
            framebuffer.fill(0x7FFF);
            frame_ready = true;
        }
    }
    updateStatInterrupt();
}

uint8_t PPU::readSTAT() const {
    uint8_t result = (stat & 0xF8) | 0x80;  // Bits 3-7
    result |= static_cast<uint8_t>(current_mode);
    result |= (ly == lyc) ? (1 << 2) : 0;
    return result;
}

void PPU::writeSTAT(uint8_t value){
    stat = (value & 0x78);
    updateStatInterrupt();  // Only bits 3-7 are writable
}

uint8_t PPU::readSCY() const { return scy; }
void PPU::writeSCY(uint8_t value){ scy = value; }

uint8_t PPU::readSCX() const { return scx; }
void PPU::writeSCX(uint8_t value){ scx = value; }

uint8_t PPU::readLY() const { return ly; }
void PPU::writeLY(uint8_t){ }  // Read-only

uint8_t PPU::readLYC() const { return lyc; }
void PPU::writeLYC(uint8_t value){ lyc = value; updateStatInterrupt(); }

uint8_t PPU::readBGP() const { return bgp; }
void PPU::writeBGP(uint8_t value){ bgp = value; }

uint8_t PPU::readOBP0() const { return obp0; }
void PPU::writeOBP0(uint8_t value){ obp0 = value; }

uint8_t PPU::readOBP1() const { return obp1; }
void PPU::writeOBP1(uint8_t value){ obp1 = value; }

uint8_t PPU::readWY() const { return wy; }
void PPU::writeWY(uint8_t value){ wy = value; }

uint8_t PPU::readWX() const { return wx; }
void PPU::writeWX(uint8_t value){ wx = value; }

void PPU::renderBackgroundScanline(){
    const int y = (scy + current_scanline) & 255;
    for(int x = 0; x < 160; ++x){
        int bx = (scx + x) & 255;
        uint16_t map = useAlternateBGMap() ? 0x9C00 : 0x9800;
        uint8_t tile = mmu.readByte(map + (y / 8) * 32 + bx / 8);
        int address = (lcdc & 0x10) ? 0x8000 + tile * 16 :
                      0x9000 + static_cast<int8_t>(tile) * 16;
        address += (y % 8) * 2;
        int bit = 7 - bx % 8;
        uint8_t color = ((mmu.readByte(address) >> bit) & 1) |
                        (((mmu.readByte(address + 1) >> bit) & 1) << 1);
        background_colors[x] = color;
        framebuffer[current_scanline * 160 + x] = colorToRGB555(color, bgp);
    }
}

void PPU::renderWindowScanline(){
    if(!isWindowEnabled() || current_scanline < wy || wx > 166) return;
    int left = static_cast<int>(wx) - 7;
    for(int x = std::max(0, left); x < 160; ++x){
        int local_x = x - left;
        uint16_t map = useAlternateWindowMap() ? 0x9C00 : 0x9800;
        uint8_t tile = mmu.readByte(map + (window_line / 8) * 32 + local_x / 8);
        int address = (lcdc & 0x10) ? 0x8000 + tile * 16 :
                      0x9000 + static_cast<int8_t>(tile) * 16;
        address += (window_line % 8) * 2;
        int bit = 7 - local_x % 8;
        uint8_t color = ((mmu.readByte(address) >> bit) & 1) |
                        (((mmu.readByte(address + 1) >> bit) & 1) << 1);
        background_colors[x] = color;
        framebuffer[current_scanline * 160 + x] = colorToRGB555(color, bgp);
    }
    ++window_line;
}

void PPU::renderSpriteScanline(){
    struct Sprite { int x, y; uint8_t tile, flags; };
    std::array<Sprite, 10> selected{};
    int count = 0, height = useLargeSpriteSize() ? 16 : 8;
    // Select the first ten intersecting OAM entries, even if horizontally hidden.
    for(int i = 0; i < 40 && count < 10; ++i){
        int address = 0xFE00 + i * 4;
        int y = static_cast<int>(mmu.readByte(address)) - 16;
        if(current_scanline < y || current_scanline >= y + height) continue;
        selected[count++] = {static_cast<int>(mmu.readByte(address + 1)) - 8,
                             y, mmu.readByte(address + 2), mmu.readByte(address + 3)};
    }
    std::stable_sort(selected.begin(), selected.begin() + count,
                     [](const Sprite& a, const Sprite& b){ return a.x < b.x; });
    std::array<bool, 160> claimed{};
    for(int i = 0; i < count; ++i){
        const auto& sprite = selected[i];
        int row = current_scanline - sprite.y;
        if(sprite.flags & 0x40) row = height - 1 - row;
        int tile = height == 16 ? sprite.tile & 0xFE : sprite.tile;
        int address = 0x8000 + tile * 16 + row * 2;
        uint8_t low = mmu.readByte(address), high = mmu.readByte(address + 1);
        for(int dx = 0; dx < 8; ++dx){
            int x = sprite.x + dx;
            if(x < 0 || x >= 160 || claimed[x]) continue;
            int bit = (sprite.flags & 0x20) ? dx : 7 - dx;
            uint8_t color = ((low >> bit) & 1) | (((high >> bit) & 1) << 1);
            if(!color) continue;
            claimed[x] = true;
            if((sprite.flags & 0x80) && background_colors[x]) continue;
            framebuffer[current_scanline * 160 + x] =
                colorToRGB555(color, (sprite.flags & 0x10) ? obp1 : obp0);
        }
    }
}
