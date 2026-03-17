#include "PPU.h"
#include "MMU.h"
#include <cstring>

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
    framebuffer.fill(0xFFFF);  // White
}

void PPU::update(int cycles){
    if(!isPPUEnabled()){
        return;
    }
    
    cycles_in_mode += cycles;
    
    switch(current_mode){
        case OAM_SEARCH:
            // 80 cycles
            if(cycles_in_mode >= 80){
                cycles_in_mode -= 80;
                current_mode = PIXEL_TRANSFER;
            }
            break;
            
        case PIXEL_TRANSFER:
            // 172-289 cycles(varies)
            if(cycles_in_mode >= 172){
                cycles_in_mode -= 172;
                current_mode = HBLANK;
                // Render this scanline
                renderScanline();
            }
            break;
            
        case HBLANK:
            // 87 cycles
            if(cycles_in_mode >= 87){
                cycles_in_mode -= 87;
                current_scanline++;
                ly = current_scanline;
                
                // Check LY==LYC interrupt
                if(ly == lyc && getLYCInterrupt()){
                    lcd_stat_interrupt = true;
                }
                
                if(current_scanline >= 144){
                    // Enter V-Blank
                    current_mode = VBLANK;
                    vblank_interrupt = true;
                    frame_ready = true;
                    
                    if(getVBlankStatInterrupt()){
                        lcd_stat_interrupt = true;
                    }
                } else {
                    // Next scanline
                    current_mode = OAM_SEARCH;
                    
                    if(getOAMInterrupt()){
                        lcd_stat_interrupt = true;
                    }
                }
            }
            break;
            
        case VBLANK:
            // 456 cycles per line
            if(cycles_in_mode >= 456){
                cycles_in_mode -= 456;
                current_scanline++;
                ly = current_scanline;
                
                if(current_scanline >= 154){
                    // Back to line 0
                    current_scanline = 0;
                    ly = 0;
                    current_mode = OAM_SEARCH;
                    
                    if(getOAMInterrupt()){
                        lcd_stat_interrupt = true;
                    }
                }
            }
            break;
    }
}

void PPU::renderScanline(){
    if(current_scanline >= 144) return;
    
    // Simple background rendering for now
    uint16_t* scanline_ptr = framebuffer.data() + (current_scanline * 160);
    
    // Calculate which tile row we're in
    uint8_t scroll_y = scy + current_scanline;
    uint8_t tile_row = scroll_y / 8;
    uint8_t tile_pixel_y = scroll_y % 8;
    
    uint16_t tile_map_addr;
    if(useAlternateBGMap()){
        tile_map_addr = 0x9C00;  // Alternate map
    } else {
        tile_map_addr = 0x9800;  // Default map
    }
    
    // Render each pixel across the scanline
    for(uint16_t x = 0; x < 160; x++){
        uint8_t scroll_x = scx + x;
        uint8_t tile_col = scroll_x / 8;
        uint8_t tile_pixel_x = scroll_x % 8;
        
        // Get tile index from map
        uint16_t map_addr = tile_map_addr + (tile_row % 32) * 32 + (tile_col % 32);
        uint8_t tile_index = mmu.readByte(map_addr);
        
        // Get tile data
        uint16_t tile_data_addr;
        if((lcdc & (1 << 4)) == 0){
            // Tile data at 0x8800-0x97FF(contains signed tile numbers)
            tile_data_addr = 0x8800 + ((static_cast<int8_t>(tile_index)) + 128) * 16;
        } else {
            // Tile data at 0x8000-0x8FFF(unsigned tile numbers)
            tile_data_addr = 0x8000 + tile_index * 16;
        }
        
        // Get pixel from tile
        uint8_t tile_line_addr = tile_data_addr + tile_pixel_y * 2;
        uint8_t low = mmu.readByte(tile_line_addr);
        uint8_t high = mmu.readByte(tile_line_addr + 1);
        
        // Extract pixel(bit 7 is leftmost)
        uint8_t pixel_bit = 7 - tile_pixel_x;
        uint8_t palette_index = ((high >> pixel_bit) & 1) << 1 | ((low >> pixel_bit) & 1);
        
        // Convert palette to color
        scanline_ptr[x] = colorToRGB555(palette_index, bgp);
    }
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
void PPU::writeLCDC(uint8_t value){ lcdc = value; }

uint8_t PPU::readSTAT() const {
    uint8_t result = stat & 0xF8;  // Bits 3-7
    result |= static_cast<uint8_t>(current_mode);
    result |= (ly == lyc) ? (1 << 2) : 0;
    return result;
}

void PPU::writeSTAT(uint8_t value){
    stat = (value & 0xF8);  // Only bits 3-7 are writable
}

uint8_t PPU::readSCY() const { return scy; }
void PPU::writeSCY(uint8_t value){ scy = value; }

uint8_t PPU::readSCX() const { return scx; }
void PPU::writeSCX(uint8_t value){ scx = value; }

uint8_t PPU::readLY() const { return ly; }
void PPU::writeLY(uint8_t value){ ly = 0; }  // Writing resets to 0

uint8_t PPU::readLYC() const { return lyc; }
void PPU::writeLYC(uint8_t value){ lyc = value; }

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
    // Placeholder for future full background rendering
}

void PPU::renderWindowScanline(){
    // Placeholder for future window rendering
}

void PPU::renderSpriteScanline(){
    // Placeholder for future sprite rendering
}
