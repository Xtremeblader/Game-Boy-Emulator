#include "Cartridge.h"
#include <filesystem>
#include <fstream>
#include <cassert>
#include <iostream>
#include <vector>
#include <unistd.h>
namespace fs = std::filesystem;
void rom(const fs::path& path, int type){
    std::vector<char> bytes(32768,0);
    bytes[0x147]=type; bytes[0x149]=3;
    std::ofstream f(path,std::ios::binary); f.write(bytes.data(),bytes.size());
}
std::vector<char> read(const fs::path& p){
    std::ifstream f(p,std::ios::binary);
    return {std::istreambuf_iterator<char>(f),std::istreambuf_iterator<char>()};
}
int main(){
    char pattern[]="/tmp/gb-battery-XXXXXX";
    auto name=mkdtemp(pattern); assert(name);
    fs::path dir(name), game=dir/"Test Game.gb", save=dir/"Test Game.sav";
    rom(game,0x13);
    {
        Cartridge c; assert(c.loadFromFile(game.string()));
        assert(c.saveBattery() && !fs::exists(save));
        c.writeRomByte(0,0x0A);
        for(int bank=0;bank<4;bank++){
            c.writeRomByte(0x4000,bank);
            c.writeRamByte(0xA000,0x40+bank);
            c.writeRamByte(0xBFFF,0x80+bank);
        }
        assert(c.saveBattery());
        assert(fs::file_size(save)==32768);
        auto stamp=fs::last_write_time(save);
        assert(c.saveBattery() && fs::last_write_time(save)==stamp);
        c.writeRamByte(0xA001,0x99); // Destructor flushes dirty RAM.
    }
    {
        Cartridge c; assert(c.loadFromFile(game.string()));
        c.writeRomByte(0,0x0A);
        for(int bank=0;bank<4;bank++){
            c.writeRomByte(0x4000,bank);
            assert(c.readRamByte(0xA000)==0x40+bank);
            assert(c.readRamByte(0xBFFF)==0x80+bank);
        }
        assert(c.readRamByte(0xA001)==0x99);
        // A failed save preserves the existing file and stays dirty for retry.
        fs::rename(dir,dir.string()+"-moved");
        c.writeRamByte(0xA001,0x55);
        assert(!c.saveBattery());
        auto old=read(fs::path(dir.string()+"-moved")/save.filename());
        assert(static_cast<unsigned char>(old[3*8192+1])==0x99);
        fs::rename(dir.string()+"-moved",dir);
        assert(c.saveBattery());
    }
    auto volatile_game=dir/"Volatile.gb"; rom(volatile_game,0x12);
    {Cartridge c; assert(c.loadFromFile(volatile_game.string()));
     c.writeRomByte(0,10); c.writeRamByte(0xA000,55); assert(c.saveBattery());}
    assert(!fs::exists(dir/"Volatile.sav"));
    // Never overwrite a truncated or oversized existing save.
    for(int size : {7,32769}){
        std::vector<char> bad(size,42);
        {std::ofstream f(save,std::ios::binary|std::ios::trunc); f.write(bad.data(),bad.size());}
        {Cartridge c; assert(!c.loadFromFile(game.string())); assert(c.saveBattery());}
        assert(read(save)==bad);
    }
    // Direct ROM+RAM battery cartridges need no RAM-enable command.
    auto direct=dir/"Direct.gb"; rom(direct,9);
    {Cartridge c; assert(c.loadFromFile(direct.string()));c.writeRamByte(0xA000,77);}
    {Cartridge c; assert(c.loadFromFile(direct.string()));assert(c.readRamByte(0xA000)==77);}
    fs::remove_all(dir);
    std::cout<<"Battery save round-trip, banks, failure/retry, and protection tests passed\n";
}
