#include "MMU.h"
#include "Cartridge.h"
#include <iostream>
#include <fstream>
#include <cstring>

// Helper to create a minimal Game Boy ROM for testing
bool createTestRom(const std::string& filename){
    std::ofstream file(filename, std::ios::binary);
    if(!file.is_open()) return false;
    
    // Create a 32KB ROM(minimum size)
    std::vector<uint8_t> rom(0x8000, 0xFF);
    
    // Set entry point(0x100-0x103)
    rom[0x100] = 0x00;  // NOP
    rom[0x101] = 0xC3;  // JP
    rom[0x102] = 0x50;  // Address low
    rom[0x103] = 0x01;  // Address high
    
    // Some test data
    rom[0x150] = 0x3C;  // INC A
    rom[0x151] = 0xC9;  // RET
    
    // Set cartridge header
    std::string title = "TEST";
    std::memcpy(&rom[0x134], title.c_str(), 4);
    
    rom[0x147] = 0x00;  // ROM-only
    rom[0x148] = 0x00;  // 32KB(2 banks)
    rom[0x149] = 0x00;  // No RAM
    rom[0x14B] = 0x01;  // Non-Japanese
    rom[0x14C] = 0x00;  // Version
    
    // Calculate and set header checksum(simplified)
    uint8_t checksum = 0;
    for(int i = 0x134; i <= 0x14C; i++){
        checksum = checksum - rom[i] - 1;
    }
    rom[0x14D] = checksum;
    
    // Write to file
    file.write(reinterpret_cast<char*>(rom.data()), rom.size());
    file.close();
    return true;
}

int main(){
    std::cout << "=== Cartridge & ROM Loading Tests ===" << std::endl << std::endl;
    
    // Test 1: Create and load a test ROM
    std::cout << "Test 1: Create and load test ROM" << std::endl;
    if(!createTestRom("test_rom.gb")){
        std::cerr << "Failed to create test ROM" << std::endl;
        return 1;
    }
    std::cout << "✓ Test ROM created" << std::endl;
    std::cout << std::endl;
    
    // Test 2: Load cartridge via MMU
    std::cout << "Test 2: Load cartridge via MMU" << std::endl;
    MMU mmu;
    if(!mmu.loadCartridge("test_rom.gb")){
        std::cerr << "Failed to load cartridge" << std::endl;
        return 1;
    }
    std::cout << "✓ Cartridge loaded successfully" << std::endl;
    std::cout << std::endl;
    
    // Test 3: Read from ROM
    std::cout << "Test 3: Read from ROM areas" << std::endl;
    uint8_t val1 = mmu.readByte(0x100);
    uint8_t val2 = mmu.readByte(0x150);
    std::cout << "Value at 0x0100: 0x" << std::hex << (int)val1 << std::dec;
    if(val1 == 0x00){
        std::cout << " ✓" << std::endl;
    } else {
        std::cout << " ✗ (expected 0x00)" << std::endl;
    }
    
    std::cout << "Value at 0x0150: 0x" << std::hex << (int)val2 << std::dec;
    if(val2 == 0x3C){
        std::cout << " ✓" << std::endl;
    } else {
        std::cout << " ✗ (expected 0x3C)" << std::endl;
    }
    std::cout << std::endl;
    
    // Test 4: Write to and read from RAM
    std::cout << "Test 4: Write to Work RAM" << std::endl;
    mmu.writeByte(0xC000, 0x42);
    uint8_t ram_val = mmu.readByte(0xC000);
    std::cout << "Written 0x42 to 0xC000, read back: 0x" << std::hex << (int)ram_val << std::dec;
    if(ram_val == 0x42){
        std::cout << " ✓" << std::endl;
    } else {
        std::cout << " ✗" << std::endl;
    }
    std::cout << std::endl;
    
    // Test 5: Memory mapping verification
    std::cout << "Test 5: Memory area isolation" << std::endl;
    mmu.writeByte(0xC000, 0xAA);  // WRAM
    mmu.writeByte(0x8000, 0xBB);  // VRAM
    
    if(mmu.readByte(0xC000) == 0xAA && mmu.readByte(0x8000) == 0xBB){
        std::cout << "✓ WRAM and VRAM are properly isolated" << std::endl;
    } else {
        std::cout << "✗ Memory areas overlapping" << std::endl;
    }
    std::cout << std::endl;
    
    // Test 6: Cartridge info
    std::cout << "Test 6: Cartridge information" << std::endl;
    Cartridge* cart = mmu.getCartridge();
    if(cart){
        std::cout << "Title: " << cart->getTitle() << std::endl;
        std::cout << "ROM Banks: " << cart->getRomBanks() << std::endl;
        std::cout << "RAM Size: " << cart->getRamSize() << std::endl;
        if(cart->getTitle() == "TEST" && cart->getRomBanks() == 2){
            std::cout << "✓ Cartridge info correct" << std::endl;
        }
    }
    std::cout << std::endl;
    
    std::cout << "=== ROM Loading Tests Complete ===" << std::endl;
    return 0;
}
