#include "CPU.h"
#include "Timer.h"
#include <cassert>
#include <iostream>
int main(){
    MMU mmu;
    CPU cpu(mmu);
    Timer timer;
    mmu.linkTimer(&timer);
    cpu.PC = 0xC000;
    // NOPs in WRAM advance DIV using the CPU's returned clock count.
    for(int i = 0; i < 64; ++i) mmu.updateTimer(cpu.step());
    assert(mmu.readByte(0xFF04) == 1);
    mmu.writeByte(0xFF04, 123);
    assert(mmu.readByte(0xFF04) == 0);
    mmu.writeByte(0xFF05, 0xFF);
    mmu.writeByte(0xFF06, 0x42);
    mmu.writeByte(0xFF07, 0xFD);
    assert(mmu.readByte(0xFF05) == 0xFF && mmu.readByte(0xFF06) == 0x42);
    assert(mmu.readByte(0xFF07) == 0xFD);
    cpu.IF = 1;
    mmu.updateTimer(16);
    assert(cpu.IF == 1 && mmu.readByte(0xFF05) == 0);
    mmu.updateTimer(4);
    assert(cpu.IF == 5 && mmu.readByte(0xFF05) == 0x42);
    cpu.IE = 4;
    cpu.IME = true;
    cpu.SP = 0xD000;
    uint16_t interrupted = cpu.PC;
    assert(cpu.step() == 20 && cpu.PC == 0x50 && cpu.IF == 1);
    assert(mmu.readWord(cpu.SP) == interrupted);
    mmu.updateTimer(0);
    assert(cpu.IF == 1); // Interrupt isn't reasserted after CPU acknowledgement.
    std::cout << "Timer MMU mapping and CPU interrupt integration passed\n";
}
