#pragma once
#include <cstdint>
#include <array>
#include <string>
#include <memory>

class Cartridge;

class MMU {
public:
    MMU();
    ~MMU();
    
    // Cartridge management
    bool loadCartridge(const std::string& filepath);
    Cartridge* getCartridge() const { return cartridge.get(); }
    
    // Link interrupt registers for I/O mapping
    void linkInterruptRegisters(uint8_t* ie_ptr, uint8_t* if_ptr);
    
    // read and write functions for CPU
    uint8_t readByte(uint16_t address) const;
    void writeByte(uint16_t address, uint8_t value);

    // helpers for r/w 16-bit values
    uint16_t readWord(uint16_t address) const;
    void writeWord(uint16_t address, uint16_t value);

    // Legacy ROM loading(will be replaced by loadCartridge)
    bool loadROM(const std::string& filepath);

private:
    std::unique_ptr<Cartridge> cartridge;
    
    // Internal RAM and I/O
    // 0x0000-0x3FFF: ROM Bank 0 (from cartridge)
    // 0x4000-0x7FFF: ROM Bank N(from cartridge, switchable)
    // 0x8000-0x9FFF: VRAM(video RAM)
    // 0xA000-0xBFFF: External RAM(cartridge RAM)
    // 0xC000-0xDFFF: Work RAM
    // 0xE000-0xFDFF: Echo of C000-DDFF
    // 0xFE00-0xFE9F: OAM(sprite data)
    // 0xFF00-0xFFFF: I/O Registers
    
    std::array<uint8_t, 0x2000> vram;        // Video RAM 0x8000-0x9FFF
    std::array<uint8_t, 0x2000> wram;        // Work RAM 0xC000-0xDFFF
    std::array<uint8_t, 0xA0> oam;           // OAM 0xFE00-0xFE9F
    std::array<uint8_t, 0x80> io_registers;  // I/O 0xFF00-0xFF7F
    std::array<uint8_t, 0x80> hram;          // High RAM 0xFF80-0xFFFF
    
    // Pointers to CPU interrupt registers(for I/O mapping)
    uint8_t* IE_ptr = nullptr;  // 0xFFFF
    uint8_t* IF_ptr = nullptr;  // 0xFF0F
};