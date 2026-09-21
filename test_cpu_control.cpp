#include "CPU.h"
#include <cassert>
#include <iostream>

int main(){
    // Execute synthetic programs in writable RAM; ROM writes cannot install code.
    MMU mmu;
    CPU cpu(mmu);
    cpu.PC = 0xC000;
    cpu.SP = 0xD000;
    mmu.writeByte(0xC000, 0xCD); // CALL C010
    mmu.writeWord(0xC001, 0xC010);
    mmu.writeByte(0xC010, 0xCD); // CALL C020
    mmu.writeWord(0xC011, 0xC020);
    mmu.writeByte(0xC020, 0xC9);
    mmu.writeByte(0xC013, 0xC9);
    assert(cpu.step() == 24 && cpu.PC == 0xC010 && cpu.SP == 0xCFFE);
    assert(mmu.readWord(cpu.SP) == 0xC003);
    assert(cpu.step() == 24 && cpu.PC == 0xC020 && cpu.SP == 0xCFFC);
    assert(mmu.readWord(cpu.SP) == 0xC013);
    assert(cpu.step() == 16 && cpu.PC == 0xC013);
    assert(cpu.step() == 16 && cpu.PC == 0xC003 && cpu.SP == 0xD000);

    const uint8_t opcodes[] = {0xC0, 0xC8, 0xD0, 0xD8};
    const uint8_t flags[] = {0x80, 0x00, 0x10, 0x00};
    for(int i = 0; i < 4; ++i){
        cpu.PC = 0xC000;
        cpu.SP = 0xD000;
        cpu.AF.bytes.lo = flags[i];
        mmu.writeByte(cpu.PC, opcodes[i]);
        mmu.writeWord(cpu.SP, 0xC123);
        assert(cpu.step() == 8 && cpu.PC == 0xC001 && cpu.SP == 0xD000);
        assert(mmu.readWord(cpu.SP) == 0xC123);
        cpu.PC = 0xC000;
        cpu.AF.bytes.lo ^= i < 2 ? 0x80 : 0x10;
        assert(cpu.step() == 20 && cpu.PC == 0xC123 && cpu.SP == 0xD002);
    }
    cpu.PC = 0xC000;
    cpu.SP = 0xD000;
    cpu.IME = false;
    mmu.writeByte(cpu.PC, 0xD9); // RETI
    mmu.writeWord(cpu.SP, 0xC123);
    assert(cpu.step() == 16 && cpu.PC == 0xC123 && cpu.IME);
    cpu.IE = cpu.IF = 1;
    assert(cpu.step() == 20 && cpu.PC == 0x40 && !cpu.IME);
    assert(cpu.SP == 0xD000 && mmu.readWord(cpu.SP) == 0xC123);

    for(uint8_t selection : {0x00, 0x10, 0x20, 0x30}){
        mmu.writeByte(0xFF00, selection);
        assert(mmu.readByte(0xFF00) == (selection | 0xCF));
    }
    std::cout << "CPU control-flow and released-joypad tests passed\n";
}
