#include "Timer.h"
#include <cassert>
#include <iostream>

int main(){
    Timer timer;
    timer.update(255);
    assert(timer.readDIV() == 0 && timer.readTIMA() == 0);
    timer.update(1);
    assert(timer.readDIV() == 1 && timer.readTIMA() == 0);
    timer.writeDIV(0xAB);
    assert(timer.readDIV() == 0);
    timer.update(65536);
    assert(timer.readDIV() == 0);
    const int periods[] = {1024, 16, 64, 256};
    for(int mode = 0; mode < 4; ++mode){
        Timer t;
        t.writeTAC(4 | mode);
        assert(t.readTAC() == (0xFC | mode));
        t.update(periods[mode] - 1);
        assert(t.readTIMA() == 0);
        t.update(1);
        assert(t.readTIMA() == 1);
    }
    Timer edge;
    edge.writeTAC(5);
    edge.update(8);
    edge.writeDIV(0);
    assert(edge.readTIMA() == 1); // DIV reset creates falling edge.
    edge.update(8);
    edge.writeTAC(0);
    assert(edge.readTIMA() == 2); // Disabling selected high input also ticks.
    edge.writeTAC(5);
    edge.writeTAC(6);
    assert(edge.readTIMA() == 3); // Frequency change high -> low.

    Timer overflow;
    overflow.writeTMA(0x42);
    overflow.writeTIMA(0xFF);
    overflow.writeTAC(5);
    overflow.update(16);
    assert(overflow.readTIMA() == 0 && !overflow.hasInterrupt());
    overflow.update(3);
    assert(overflow.readTIMA() == 0 && !overflow.hasInterrupt());
    overflow.writeTMA(0x51);
    overflow.update(1);
    assert(overflow.readTIMA() == 0x51 && overflow.hasInterrupt());
    overflow.clearInterrupt();
    assert(!overflow.hasInterrupt());

    Timer cancel;
    cancel.writeTIMA(0xFF);
    cancel.writeTAC(5);
    cancel.update(16);
    cancel.writeTIMA(0x23);
    cancel.update(4);
    assert(cancel.readTIMA() == 0x23 && !cancel.hasInterrupt());

    Timer disabled;
    disabled.writeTIMA(0xFF);
    disabled.writeTMA(0x55);
    disabled.writeTAC(5);
    disabled.update(16);
    disabled.writeTAC(0);
    disabled.update(4);
    assert(disabled.readTIMA() == 0x55 && disabled.hasInterrupt());
    std::cout << "Timer frequency, divider, edge, and overflow tests passed\n";
}
