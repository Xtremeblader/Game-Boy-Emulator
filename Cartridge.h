#pragma once
#include <cstdint>
#include <string>
#include <vector>

class Cartridge {
public:
    enum CartridgeType {
        ROM_ONLY = 0x00,
        MBC1 = 0x01,
        MBC1_RAM = 0x02,
        MBC1_RAM_BATTERY = 0x03,
        MBC2 = 0x05,
        MBC2_BATTERY = 0x06,
        ROM_RAM = 0x08,
        ROM_RAM_BATTERY = 0x09,
        MMM01 = 0x0B,
        MBC3_TIMER_BATTERY = 0x0F,
        MBC3_TIMER_RAM_BATTERY = 0x10,
        MBC3 = 0x11,
        MBC3_RAM = 0x12,
        MBC3_RAM_BATTERY = 0x13,
        MBC5 = 0x19,
        MBC5_RAM = 0x1A,
        MBC5_RAM_BATTERY = 0x1B,
        MBC5_RUMBLE = 0x1C,
        MBC5_RUMBLE_RAM = 0x1D,
        MBC5_RUMBLE_RAM_BATTERY = 0x1E,
    };
    
    enum RamSize {
        RAM_NONE = 0,
        RAM_2KB = 0x0800,      // MBC2(512 x 4-bit)
        RAM_8KB = 0x2000,
        RAM_32KB = 0x8000,
        RAM_128KB = 0x20000,
        RAM_64KB = 0x10000,
    };
    
    Cartridge() = default;
    Cartridge(const Cartridge&) = delete;
    Cartridge& operator=(const Cartridge&) = delete;
    ~Cartridge();
    bool saveBattery();
    const std::string& getSavePath() const { return save_path; }

    bool loadFromFile(const std::string& filepath);
    
    // Accessors
    const std::string& getTitle() const { return title; }
    CartridgeType getType() const { return type; }
    int getRomBanks() const { return rom_banks; }
    int getRamSize() const { return ram_size; }
    bool hasBattery() const { return has_battery; }
    bool hasRam() const { return has_ram; }
    
    // ROM data access
    uint8_t readRomByte(uint16_t address) const;
    void writeRomByte(uint16_t address, uint8_t value);  // For MBC register writes
    
    // RAM data access
    uint8_t readRamByte(uint16_t address) const;
    void writeRamByte(uint16_t address, uint8_t value);
    
    // Bank control
    int getCurrentRomBank() const { return current_rom_bank; }
    int getCurrentRamBank() const { return current_ram_bank; }
    void setRomBank(int bank);
    void setRamBank(int bank);
    void enableRam(bool enable){ ram_enabled = enable; }
    bool isRamEnabled() const { return ram_enabled; }
    
private:
    std::string save_path;
    bool save_ready = false;
    bool ram_dirty = false;
    bool loadBattery();

    // ROM data
    std::vector<uint8_t> rom_data;
    std::vector<uint8_t> ram_data;
    
    // Cartridge info from header
    std::string title;
    CartridgeType type;
    int rom_banks;
    int ram_size;
    bool has_battery;
    bool has_ram;
    
    // Banking state
    int current_rom_bank;
    int current_ram_bank;
    bool ram_enabled;
    
    // Helper methods
    CartridgeType parseCartridgeType(uint8_t type_byte);
    int parseRomSize(uint8_t rom_size_byte);
    RamSize parseRamSize(uint8_t ram_size_byte);
    bool validateChecksum();
};
