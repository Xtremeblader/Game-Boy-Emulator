#include "MMU.h"
#include "CPU.h"
#include <iostream>
#include <string>

int main(int argc, char* argv[]){
    MMU mmu;
    CPU cpu(mmu);

    // Get ROM path from command line or use default
    std::string rom_path = "Tetris(JUE) (V1.1) [!].gb";
    if(argc > 1){
        rom_path = argv[1];
    }

    if(!mmu.loadCartridge(rom_path)){
        std::cerr << "Failed to load cartridge: " << rom_path << std::endl;
        std::cerr << "Usage: " << argv[0] << " [path_to_rom]" << std::endl;
        return 1;
    }

    std::cout << "Starting emulation... (Press Ctrl+C to stop)" << std::endl;
    
    bool running = true;
    int cycle_count = 0;
    
    // Simple emulation loop(no timing yet)
    while(running && cycle_count < 1000000){
        cpu.step();
        cycle_count++;
        
        // For now, just run for 1M cycles and stop
        if(cycle_count % 100000 == 0){
            std::cout << "Executed " << cycle_count << " cycles, PC: 0x" << std::hex << cpu.PC << std::dec << std::endl;
        }
    }

    std::cout << "Emulation stopped. Total cycles: " << cycle_count << std::endl;
    return 0;
}