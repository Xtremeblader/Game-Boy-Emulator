#pragma once
#include <cstdint>
#include <array>
#include <memory>

class MMU;

class PPU {
public:
    PPU(MMU& mmu_ref);
    
    // Update PPU for the given number of cycles
    void update(int cycles);
    
    // PPU mode/state
    enum PPUMode {
        HBLANK = 0,      // Horizontal blank(0-87 cycles)
        VBLANK = 1,      // Vertical blank(2288-4563 cycles)
        OAM_SEARCH = 2,  // OAM search(80-252 cycles)
        PIXEL_TRANSFER = 3  // Pixel transfer(172-289 cycles)
    };
    
    PPUMode getMode() const { return current_mode; }
    uint8_t getCurrentScanline() const { return current_scanline; }
    
    // Framebuffer(160x144 pixels, 2 bytes per pixel for color)
    // Each pixel is stored as a 16-bit value in 555 RGB format
    // Use getFramebuffer() to access
    const std::array<uint16_t, 160 * 144>& getFramebuffer() const { return framebuffer; }
    bool hasNewFrame() const { return frame_ready; }
    void resetFrameReady(){ frame_ready = false; }
    
    // Interrupt signals(checked externally by CPU)
    bool getVBlankInterrupt() const { return vblank_interrupt; }
    bool getLcdStatInterrupt() const { return lcd_stat_interrupt; }
    void clearVBlankInterrupt(){ vblank_interrupt = false; }
    void clearLcdStatInterrupt(){ lcd_stat_interrupt = false; }
    
    // LCDC register bits
    enum LcdcBit {
        PPU_ENABLE = 7,
        WINDOW_TILE_MAP = 6,
        WINDOW_ENABLE = 5,
        BG_TILE_DATA = 4,
        BG_TILE_MAP = 3,
        SPRITE_SIZE = 2,
        SPRITE_ENABLE = 1,
        BG_WIN_PRIORITY = 0
    };
    
    // I/O Register accessors
    uint8_t readLCDC() const;
    void writeLCDC(uint8_t value);
    uint8_t readSTAT() const;
    void writeSTAT(uint8_t value);
    uint8_t readSCY() const;
    void writeSCY(uint8_t value);
    uint8_t readSCX() const;
    void writeSCX(uint8_t value);
    uint8_t readLY() const;
    void writeLY(uint8_t value);
    uint8_t readLYC() const;
    void writeLYC(uint8_t value);
    uint8_t readBGP() const;
    void writeBGP(uint8_t value);
    uint8_t readOBP0() const;
    void writeOBP0(uint8_t value);
    uint8_t readOBP1() const;
    void writeOBP1(uint8_t value);
    uint8_t readWY() const;
    void writeWY(uint8_t value);
    uint8_t readWX() const;
    void writeWX(uint8_t value);
    
private:
    MMU& mmu;
    
    // PPU state
    PPUMode current_mode;
    int cycles_in_mode;
    uint8_t current_scanline;
    
    // PPU registers(0xFF40-0xFF4B)
    uint8_t lcdc;  // 0xFF40 - LCD Control
    uint8_t stat;  // 0xFF41 - LCD Status
    uint8_t scy;   // 0xFF42 - Scroll Y
    uint8_t scx;   // 0xFF43 - Scroll X
    uint8_t ly;    // 0xFF44 - Current scanline(read-only)
    uint8_t lyc;   // 0xFF45 - LY Compare
    uint8_t bgp;   // 0xFF47 - BG Palette
    uint8_t obp0;  // 0xFF48 - OBJ Palette 0
    uint8_t obp1;  // 0xFF49 - OBJ Palette 1
    uint8_t wy;    // 0xFF4A - Window Y
    uint8_t wx;    // 0xFF4B - Window X
    
    // Framebuffer
    std::array<uint16_t, 160 * 144> framebuffer;
    bool frame_ready;
    
    // Interrupt flags
    bool vblank_interrupt;
    bool lcd_stat_interrupt;
    
    // Rendering helpers
    void renderScanline();
    void renderBackgroundScanline();
    void renderWindowScanline();
    void renderSpriteScanline();
    uint16_t colorToRGB555(uint8_t color, uint8_t palette);
    
    // Flag accessors from LCDC
    bool isPPUEnabled() const { return(lcdc & (1 << PPU_ENABLE)) != 0; }
    bool useAlternateWindowMap() const { return(lcdc & (1 << WINDOW_TILE_MAP)) != 0; }
    bool isWindowEnabled() const { return(lcdc & (1 << WINDOW_ENABLE)) != 0; }
    bool useAlternateBGMap() const { return(lcdc & (1 << BG_TILE_MAP)) != 0; }
    bool useLargeSpriteSize() const { return(lcdc & (1 << SPRITE_SIZE)) != 0; }
    bool spritesEnabled() const { return(lcdc & (1 << SPRITE_ENABLE)) != 0; }
    bool backgroundEnabled() const { return(lcdc & (1 << BG_WIN_PRIORITY)) != 0; }
    
    // Flag accessors from STAT
    bool getLYCInterrupt() const { return(stat & (1 << 6)) != 0; }
    bool getOAMInterrupt() const { return(stat & (1 << 5)) != 0; }
    bool getVBlankStatInterrupt() const { return(stat & (1 << 4)) != 0; }
    bool getHBlankStatInterrupt() const { return(stat & (1 << 3)) != 0; }
    bool getLYCEqualsLY() const { return(stat & (1 << 2)) != 0; }
};
