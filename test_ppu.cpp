#include "PPU.h"
#include "MMU.h"
#include <iostream>

int main(){
    std::cout << "=== PPU Initialization Tests ===" << std::endl << std::endl;
    
    // Test 1: PPU initialization
    std::cout << "Test 1: PPU initialization" << std::endl;
    MMU mmu;
    PPU ppu(mmu);
    
    std::cout << "✓ PPU created" << std::endl;
    std::cout << "Initial mode: " << (int)ppu.getMode() << " (2=OAM_SEARCH)" << std::endl;
    std::cout << "Current scanline: " << (int)ppu.getCurrentScanline() << std::endl;
    std::cout << std::endl;
    
    // Test 2: PPU registers(read/write)
    std::cout << "Test 2: PPU register access" << std::endl;
    ppu.writeLCDC(0x91);
    if(ppu.readLCDC() == 0x91){
        std::cout << "✓ LCDC register read/write" << std::endl;
    }
    
    ppu.writeSCX(0x42);
    if(ppu.readSCX() == 0x42){
        std::cout << "✓ SCX register read/write" << std::endl;
    }
    
    ppu.writeSCY(0x55);
    if(ppu.readSCY() == 0x55){
        std::cout << "✓ SCY register read/write" << std::endl;
    }
    std::cout << std::endl;
    
    // Test 3: PPU timing/cycles
    std::cout << "Test 3: PPU mode transitions" << std::endl;
    for(int i = 0; i < 5; i++){
        ppu.update(456);  // One full line(OAM + PIXEL + HBLANK)
        std::cout << "After " << (i+1) << " line(s): mode=" << (int)ppu.getMode() 
                  << ", scanline=" << (int)ppu.getCurrentScanline() << std::endl;
    }
    std::cout << std::endl;
    
    // Test 4: Framebuffer
    std::cout << "Test 4: Framebuffer access" << std::endl;
    const auto& fb = ppu.getFramebuffer();
    if(fb.size() == 160 * 144){
        std::cout << "✓ Framebuffer size correct(160x144)" << std::endl;
    }
    std::cout << "First pixel color: 0x" << std::hex << fb[0] << std::dec << std::endl;
    std::cout << std::endl;
    
    // Test 5: V-Blank timing
    std::cout << "Test 5: V-Blank generation" << std::endl;
    // Simulate 144 scanlines(visible area)
    for(int line = 0; line < 144; line++){
        ppu.update(456);
    }
    if(ppu.getVBlankInterrupt()){
        std::cout << "✓ V-Blank interrupt generated" << std::endl;
    }
    std::cout << "Current mode after V-Blank: " << (int)ppu.getMode() << std::endl;
    std::cout << std::endl;
    
    std::cout << "=== PPU Tests Complete ===" << std::endl;
    return 0;
}
