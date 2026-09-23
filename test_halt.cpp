#include "CPU.h"
#include "PPU.h"
#include "Timer.h"
#include <cassert>
#include <iostream>

struct Machine {
    MMU m;
    CPU c{m};
    PPU p{m};
    Timer t;
    Machine(){ c.PC=0xC000; c.SP=0xD000; m.linkPPU(&p); m.linkTimer(&t); }
    void program(std::initializer_list<uint8_t> bytes){
        int address=0xC000; for(auto b:bytes) m.writeByte(address++,b);
    }
    void tick(){
        int cycles=c.step(); m.updateTimer(cycles); m.updateDMA(cycles); p.update(cycles);
        if(p.getVBlankInterrupt()){c.IF|=1; p.clearVBlankInterrupt();}
    }
};
int main(){
    { // HALT idles without fetching, even with unused IE/IF bits set.
        Machine x; x.program({0x76,0x04}); x.c.IE=x.c.IF=0xE0;
        x.tick(); auto b=x.c.BC.bytes.hi;
        for(int i=0;i<114;++i)x.tick();
        assert(x.c.PC==0xC001 && x.c.BC.bytes.hi==b);
        assert(x.t.readDIV()==1 && x.p.readLY()==1);
        x.c.IE=1; x.c.IF=1; // IME off: wake without interrupt service.
        assert(x.c.step()==4 && x.c.PC==0xC002 && x.c.BC.bytes.hi==b+1);
        assert(x.c.IF==1 && x.c.SP==0xD000);
    }
    { // A real timer interrupt wakes HALT and vectors to 0050.
        Machine x; x.program({0x76,0x04}); x.c.IME=true; x.c.IE=4;
        x.t.writeTIMA(0xFF); x.t.writeTAC(5);
        for(int i=0;i<5;++i)x.tick();
        assert(x.c.PC==0xC001 && x.c.IF==4);
        assert(x.c.step()==20 && x.c.PC==0x50);
        assert(x.m.readWord(x.c.SP)==0xC001 && !x.c.IME && x.c.IF==0);
    }
    { // PPU reaches VBlank while CPU remains halted.
        Machine x; x.program({0x76}); x.c.IME=true; x.c.IE=1;
        for(int i=0;i<456*144/4;++i)x.tick();
        assert(x.c.PC==0xC001 && x.c.IF==1);
        assert(x.c.step()==20 && x.c.PC==0x40);
    }
    { // Pending interrupt with IME off: next opcode fetch doesn't increment PC.
        Machine x; x.program({0x76,0x3E,0x12}); x.c.IE=x.c.IF=1;
        assert(x.c.step()==4);
        assert(x.c.step()==8 && x.c.AF.bytes.hi==0x3E && x.c.PC==0xC002);
    }
    { // EI permits the complete next instruction before servicing an interrupt.
        Machine x; x.program({0xFB,0x3E,0x42,0}); x.c.IE=x.c.IF=1;
        assert(x.c.step()==4 && !x.c.IME);
        assert(x.c.step()==8 && x.c.IME && x.c.AF.bytes.hi==0x42);
        assert(x.c.step()==20 && x.c.PC==0x40 && x.m.readWord(x.c.SP)==0xC003);
    }
    { // DI cancels delayed EI.
        Machine x; x.program({0xFB,0xF3,0}); x.c.IE=x.c.IF=1;
        x.c.step(); x.c.step(); x.c.step();
        assert(!x.c.IME && x.c.PC==0xC003);
    }
    { // Repeated EI doesn't postpone the first EI.
        Machine x; x.program({0xFB,0xFB,0}); x.c.IE=x.c.IF=1;
        x.c.step(); x.c.step(); assert(x.c.IME);
        assert(x.c.step()==20 && !x.c.IME);
    }
    { // EI;HALT with pending IRQ returns to HALT, not a bugged handler fetch.
        Machine x; x.program({0xFB,0x76,0}); x.c.IE=x.c.IF=1;
        x.c.step(); x.c.step(); assert(x.c.IME);
        assert(x.c.step()==20 && x.m.readWord(x.c.SP)==0xC001);
        x.c.PC=0xC010; x.m.writeByte(0xC010,0xD9); // RETI in writable memory.
        assert(x.c.step()==16 && x.c.PC==0xC001 && x.c.IME);
        x.c.step(); x.c.step(); assert(x.c.PC==0xC002);
    }
    std::cout<<"HALT, peripheral clocks, wakeup, halt bug, and EI delay tests passed\n";
}
