#include "MMU.h"
#include "Cartridge.h"
#include <fstream>
#include <iostream>

MMU::MMU() : cartridge(nullptr){
    vram.fill(0);
    wram.fill(0);
    oam.fill(0);
    io_registers.fill(0);
    hram.fill(0);
}

MMU::~MMU() = default;

bool MMU::loadCartridge(const std::string& filepath){
    cartridge = std::make_unique<Cartridge>();
    return cartridge->loadFromFile(filepath);
}

void MMU::linkInterruptRegisters(uint8_t* ie_ptr, uint8_t* if_ptr){
    IE_ptr = ie_ptr;
    IF_ptr = if_ptr;
}

uint8_t MMU::readByte(uint16_t address) const {
    // ROM area(0x0000-0x7FFF)
    if(address < 0x8000){
        if(cartridge){
            return cartridge->readRomByte(address);
        }
        return 0xFF;
    }
    
    // VRAM(0x8000-0x9FFF)
    if(address < 0xA000){
        return vram[address - 0x8000];
    }
    
    // External RAM(0xA000-0xBFFF)
    if(address < 0xC000){
        if(cartridge){
            return cartridge->readRamByte(address);
        }
        return 0xFF;
    }
    
    // Work RAM(0xC000-0xDFFF)
    if(address < 0xE000){
        return wram[address - 0xC000];
    }
    
    // Echo of Work RAM(0xE000-0xFDFF)
    if(address < 0xFE00){
        return wram[address - 0xE000];
    }
    
    // OAM(0xFE00-0xFE9F)
    if(address < 0xFEA0){
        return oam[address - 0xFE00];
    }
    
    // Unusable(0xFEA0-0xFEFF)
    if(address < 0xFF00){
        return 0xFF;
    }
    
    // I/O Registers(0xFF00-0xFF7F)
    if(address < 0xFF80){
        uint8_t io_addr = address - 0xFF00;
        
        // Handle special mappings
        if(address == 0xFFFF && IE_ptr){
            return *IE_ptr;
        }
        if(address == 0xFF0F && IF_ptr){
            return *IF_ptr;
        }
        
        return io_registers[io_addr];
    }
    
    // High RAM(0xFF80-0xFFFF)
    return hram[address - 0xFF80];
}

void MMU::writeByte(uint16_t address, uint8_t value){
    // ROM area(0x0000-0x7FFF) - Used for bank switching
    if(address < 0x8000){
        if(cartridge){
            cartridge->writeRomByte(address, value);
        }
        return;
    }
    
    // VRAM(0x8000-0x9FFF)
    if(address < 0xA000){
        vram[address - 0x8000] = value;
        return;
    }
    
    // External RAM(0xA000-0xBFFF)
    if(address < 0xC000){
        if(cartridge){
            cartridge->writeRamByte(address, value);
        }
        return;
    }
    
    // Work RAM(0xC000-0xDFFF)
    if(address < 0xE000){
        wram[address - 0xC000] = value;
        return;
    }
    
    // Echo of Work RAM(0xE000-0xFDFF)
    if(address < 0xFE00){
        wram[address - 0xE000] = value;
        return;
    }
    
    // OAM(0xFE00-0xFE9F)
    if(address < 0xFEA0){
        oam[address - 0xFE00] = value;
        return;
    }
    
    // Unusable(0xFEA0-0xFEFF)
    if(address < 0xFF00){
        return;
    }
    
    // I/O Registers(0xFF00-0xFF7F)
    if(address < 0xFF80){
        // Handle special mappings
        if(address == 0xFFFF && IE_ptr){
            *IE_ptr = value;
            return;
        }
        if(address == 0xFF0F && IF_ptr){
            *IF_ptr = value;
            return;
        }
        
        io_registers[address - 0xFF00] = value;
        return;
    }
    
    // High RAM(0xFF80-0xFFFF)
    hram[address - 0xFF80] = value;
}

uint16_t MMU::readWord(uint16_t address) const {
    uint8_t lo = readByte(address);
    uint8_t hi = readByte(address + 1);
    return(static_cast<uint16_t>(hi) << 8) | lo;
}

void MMU::writeWord(uint16_t address, uint16_t value){
    uint8_t lo = value & 0xFF;
    uint8_t hi = (value >> 8) & 0xFF;
    writeByte(address, lo);
    writeByte(address + 1, hi);
}

bool MMU::loadROM(const std::string& filepath){
    // Legacy function - delegates to loadCartridge
    return loadCartridge(filepath);
}
