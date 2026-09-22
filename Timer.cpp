#include "Timer.h"

Timer::Timer() : internal_counter(0), tima(0), tma(0), tac(0), timer_interrupt(false) {}

uint16_t Timer::getTimerBit() const {
    constexpr uint16_t bits[] = {9, 3, 5, 7};
    return bits[tac & 3];
}

bool Timer::inputHigh() const {
    return (tac & 4) && (internal_counter & (1 << getTimerBit()));
}

void Timer::tick(){
    if(reload_delay) return;
    if(++tima == 0) reload_delay = 4;
}

void Timer::update(int cycles){
    // DIV runs even when TAC disables TIMA.
    if(cycles <= 0) return;
    if(!(tac & 4) && !reload_delay){
        internal_counter = static_cast<uint16_t>(internal_counter + cycles);
        return;
    }
    for(int i = 0; i < cycles; ++i){
        if(reload_delay && --reload_delay == 0){
            tima = tma;
            timer_interrupt = true;
        }
        bool previous = inputHigh();
        ++internal_counter;
        if(previous && !inputHigh()) tick();
    }
}

void Timer::writeDIV(uint8_t){
    bool previous = inputHigh();
    internal_counter = 0;
    if(previous) tick();
}

void Timer::writeTAC(uint8_t value){
    bool previous = inputHigh();
    tac = value & 7;
    if(previous && !inputHigh()) tick();
}

void Timer::writeTIMA(uint8_t value){
    tima = value;
    reload_delay = 0; // A write during the overflow delay cancels reload/IRQ.
}
