#pragma once
#include <cstdint>

class Timer {
public:
    Timer();
    
    // Update timer for the given number of cycles
    void update(int cycles);
    
    // Check if timer interrupt was generated
    bool hasInterrupt() const { return timer_interrupt; }
    void clearInterrupt(){ timer_interrupt = false; }
    
    // I/O Register accessors(0xFF04-0xFF07)
    uint8_t readDIV() const { return static_cast<uint8_t>((internal_counter >> 8) & 0xFF); }
    void writeDIV(uint8_t value);
    
    uint8_t readTIMA() const { return tima; }
    void writeTIMA(uint8_t value);
    
    uint8_t readTMA() const { return tma; }
    void writeTMA(uint8_t value){ tma = value; }
    
    uint8_t readTAC() const { return tac | 0xF8; }
    void writeTAC(uint8_t value);
    
private:
    // Internal 16-bit counter(incremented every cycle)
    uint16_t internal_counter;
    
    // Timer registers
    uint8_t tima;  // 0xFF05 - Timer counter
    uint8_t tma;   // 0xFF06 - Timer modulo(reload value)
    uint8_t tac;   // 0xFF07 - Timer control
    
    int reload_delay = 0;
    
    // Interrupt flag
    bool timer_interrupt;
    
    // Helper
    uint16_t getTimerBit() const;
    bool inputHigh() const;
    void tick();
};
