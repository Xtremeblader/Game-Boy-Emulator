#include "Cartridge.h"
#include <fstream>
#include <iostream>
#include <iomanip>

bool Cartridge::loadFromFile(const std::string& filepath){
    std::ifstream file(filepath, std::ios::binary);
    if(!file.is_open()){
        std::cerr << "Failed to open ROM: " << filepath << std::endl;
        return false;
    }
    
    // Read entire file
    file.seekg(0, std::ios::end);
    size_t file_size = file.tellg();
    file.seekg(0, std::ios::beg);
    
    rom_data.resize(file_size);
    file.read(reinterpret_cast<char*>(rom_data.data()), file_size);
    file.close();
    
    if(rom_data.size() < 0x150){
        std::cerr << "ROM too small(not a valid Game Boy ROM)" << std::endl;
        return false;
    }
    
    // Parse cartridge header(0x100-0x150)
    // Title is at 0x134-0x143(10 bytes) or 0x134-0x142 for CGB games
    title.clear();
    for(int i = 0x134; i < 0x144; i++){
        char c = static_cast<char>(rom_data[i]);
        if(c < 32 || c > 126) break;  // Stop at non-printable
        title += c;
    }
    
    // Cartridge type at 0x147
    uint8_t type_byte = rom_data[0x147];
    type = parseCartridgeType(type_byte);
    
    // ROM size at 0x148(in 16KB banks)
    uint8_t rom_size_byte = rom_data[0x148];
    rom_banks = parseRomSize(rom_size_byte);
    
    // RAM size at 0x149
    uint8_t ram_size_byte = rom_data[0x149];
    RamSize parsed_ram = parseRamSize(ram_size_byte);
    ram_size = static_cast<int>(parsed_ram);
    
    // Determine if it has battery
    has_battery = (type_byte == 0x03 || type_byte == 0x06 || type_byte == 0x09 || 
                   type_byte == 0x0F || type_byte == 0x10 || type_byte == 0x13 ||
                   type_byte == 0x1B || type_byte == 0x1E);
    
    has_ram = (type_byte == 0x02 || type_byte == 0x03 || type_byte == 0x08 || type_byte == 0x09 ||
               type_byte == 0x0C || type_byte == 0x0D || type_byte == 0x10 || type_byte == 0x12 ||
               type_byte == 0x13 || type_byte == 0x1A || type_byte == 0x1B || type_byte == 0x1D || type_byte == 0x1E);
    
    // Allocate RAM
    if(has_ram && ram_size > 0){
        ram_data.resize(ram_size, 0);
    }
    
    // Initialize banking
    current_rom_bank = 1;
    current_ram_bank = 0;
    ram_enabled = false;
    
    // Print info
    std::cout << "==== Cartridge Loaded ====" << std::endl;
    std::cout << "Title: " << title << std::endl;
    std::cout << "Type: 0x" << std::hex << std::setfill('0') << std::setw(2) << (int)type_byte << std::dec << " ";
    
    switch(type){
        case ROM_ONLY: std::cout << "(ROM only)"; break;
        case MBC1: std::cout << "(MBC1)"; break;
        case MBC1_RAM: std::cout << "(MBC1+RAM)"; break;
        case MBC1_RAM_BATTERY: std::cout << "(MBC1+RAM+Battery)"; break;
        case MBC3: std::cout << "(MBC3)"; break;
        case MBC3_RAM: std::cout << "(MBC3+RAM)"; break;
        case MBC3_RAM_BATTERY: std::cout << "(MBC3+RAM+Battery)"; break;
        case MBC5: std::cout << "(MBC5)"; break;
        case MBC5_RAM: std::cout << "(MBC5+RAM)"; break;
        case MBC5_RAM_BATTERY: std::cout << "(MBC5+RAM+Battery)"; break;
        default: std::cout << "(Unknown)"; break;
    }
    std::cout << std::endl;
    
    std::cout << "ROM: " << rom_banks << " banks(" << (rom_banks * 16) << "KB)" << std::endl;
    if(has_ram){
        std::cout << "RAM: " << ram_size << " bytes" << std::endl;
    }
    if(has_battery){
        std::cout << "Battery: Yes" << std::endl;
    }
    std::cout << "=========================" << std::endl;
    
    return true;
}

Cartridge::CartridgeType Cartridge::parseCartridgeType(uint8_t type_byte){
    return static_cast<CartridgeType>(type_byte);
}

int Cartridge::parseRomSize(uint8_t rom_size_byte){
    // Each bank is 16KB
    switch(rom_size_byte){
        case 0x00: return 2;      // 32KB
        case 0x01: return 4;      // 64KB
        case 0x02: return 8;      // 128KB
        case 0x03: return 16;     // 256KB
        case 0x04: return 32;     // 512KB
        case 0x05: return 64;     // 1MB
        case 0x06: return 128;    // 2MB
        case 0x07: return 256;    // 4MB
        case 0x52: return 72;     // 1.1MB 
        case 0x53: return 80;     // 1.2MB 
        case 0x54: return 96;     // 1.5MB
        default: return 2;
    }
}

Cartridge::RamSize Cartridge::parseRamSize(uint8_t ram_size_byte){
    switch(ram_size_byte){
        case 0x00: return RAM_NONE;
        case 0x01: return RAM_2KB;
        case 0x02: return RAM_8KB;
        case 0x03: return RAM_32KB;
        case 0x04: return RAM_128KB;
        case 0x05: return RAM_64KB;
        default: return RAM_NONE;
    }
}

uint8_t Cartridge::readRomByte(uint16_t address) const {
    if(address < 0x4000){
        // Bank 0 (fixed)
        return rom_data[address];
    } else if(address < 0x8000){
        // Switchable bank(0x4000-0x7FFF)
        // Map to correct bank
        uint32_t bank_offset = static_cast<uint32_t>(current_rom_bank) * 0x4000;
        uint32_t rom_address = bank_offset + (address - 0x4000);
        
        if(rom_address < rom_data.size()){
            return rom_data[rom_address];
        }
        return 0xFF;
    }
    return 0xFF;
}

void Cartridge::writeRomByte(uint16_t address, uint8_t value){
    // Writes to ROM area are used for bank switching(MBC1, MBC3, MBC5)
    // These don't actually write to ROM, they control bank switching
    
    if(type == MBC1){
        if(address >= 0x2000 && address < 0x4000){
            // ROM bank select(lower 5 bits)
            int bank = value & 0x1F;
            if(bank == 0) bank = 1;  // Bank 0 is invalid, use 1
            setRomBank(bank);
        } else if(address >= 0x4000 && address < 0x6000){
            // RAM bank select or upper ROM bank bits
            setRamBank(value & 0x03);
        } else if(address >= 0x6000 && address < 0x8000){
            // Banking mode select
            // 0 = ROM banking, 1 = RAM banking
        }
    } else if(type == MBC3 || type == MBC3_RAM || type == MBC3_RAM_BATTERY){
        if(address >= 0x2000 && address < 0x4000){
            // ROM bank select
            int bank = value & 0x7F;
            if(bank == 0) bank = 1;
            setRomBank(bank);
        } else if(address >= 0x4000 && address < 0x6000){
            // RAM bank or RTC select
            setRamBank(value & 0x0F);
        } else if(address >= 0x6000 && address < 0x8000){
            // Latch clock data(RTC not implemented)
        }
    } else if(type == MBC5 || type == MBC5_RAM || type == MBC5_RAM_BATTERY){
        if(address >= 0x2000 && address < 0x3000){
            // Low 8 bits of ROM bank
            setRomBank(value);
        } else if(address >= 0x3000 && address < 0x4000){
            // High bit of ROM bank(bit 0)
            int current_low = current_rom_bank & 0xFF;
            setRomBank(current_low | ((value & 0x01) << 8));
        } else if(address >= 0x4000 && address < 0x6000){
            // RAM bank select
            setRamBank(value & 0x0F);
        }
    }
    
    if(address >= 0x0000 && address < 0x2000){
        // RAM enable(all MBCs)
        ram_enabled = (value & 0x0F) == 0x0A;
    }
}

uint8_t Cartridge::readRamByte(uint16_t address) const {
    if(!ram_enabled || ram_data.empty()){
        return 0xFF;
    }
    
    if(address >= 0xA000 && address < 0xC000){
        uint16_t rel_address = address - 0xA000;
        uint32_t ram_address = static_cast<uint32_t>(current_ram_bank) * 0x2000 + rel_address;
        
        if(ram_address < ram_data.size()){
            return ram_data[ram_address];
        }
    }
    return 0xFF;
}

void Cartridge::writeRamByte(uint16_t address, uint8_t value){
    if(!ram_enabled || ram_data.empty()){
        return;
    }
    
    if(address >= 0xA000 && address < 0xC000){
        uint16_t rel_address = address - 0xA000;
        uint32_t ram_address = static_cast<uint32_t>(current_ram_bank) * 0x2000 + rel_address;
        
        if(ram_address < ram_data.size()){
            ram_data[ram_address] = value;
        }
    }
}

void Cartridge::setRomBank(int bank){
    if(bank < 1) bank = 1;
    if(bank >= rom_banks) bank = rom_banks - 1;
    current_rom_bank = bank;
}

void Cartridge::setRamBank(int bank){
    if(bank < 0) bank = 0;
    if(ram_size == 0) return;
    
    // Calculate max bank based on RAM size
    int max_bank = (ram_size + 0x1FFF) / 0x2000;
    if(bank >= max_bank) bank = max_bank - 1;
    
    current_ram_bank = bank;
}

bool Cartridge::validateChecksum(){
    // TODO: Implement checksum validation
    return true;
}
