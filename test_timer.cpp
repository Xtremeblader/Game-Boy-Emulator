#include "Timer.h"
#include <iostream>
#include <iomanip>

int main(){
    std::cout << "=== Game Boy Timer Tests ===" << std::endl << std::endl;
    
    // Test 1: Timer initialization
    std::cout << "Test 1: Timer initialization" << std::endl;
    Timer timer;
    std::cout << "DIV(internal counter): 0x" << std::hex << (int)timer.readDIV() << std::dec << std::endl;
    std::cout << "TIMA: 0x" << std::hex << (int)timer.readTIMA() << std::dec << std::endl;
    std::cout << "TMA: 0x" << std::hex << (int)timer.readTMA() << std::dec << std::endl;
    std::cout << "TAC: 0x" << std::hex << (int)timer.readTAC() << std::dec << std::endl;
    if(timer.readDIV() == 0 && timer.readTIMA() == 0){
        std::cout << "✓ Timer initialized correctly" << std::endl;
    }
    std::cout << std::endl;
    
    // Test 2: DIV register increment
    std::cout << "Test 2: DIV register increment" << std::endl;
    timer.update(256);  // Update by 256 cycles
    std::cout << "After 256 cycles, DIV: 0x" << std::hex << (int)timer.readDIV() << std::dec;
    if(timer.readDIV() == 0x01){
        std::cout << " ✓" << std::endl;
    } else {
        std::cout << " (got 0x" << std::hex << (int)timer.readDIV() << std::dec << ")" << std::endl;
    }
    std::cout << std::endl;
    
    // Test 3: DIV write resets counter
    std::cout << "Test 3: Writing to DIV resets internal counter" << std::endl;
    timer.writeDIV(0);
    std::cout << "After DIV write, DIV: 0x" << std::hex << (int)timer.readDIV() << std::dec;
    if(timer.readDIV() == 0){
        std::cout << " ✓" << std::endl;
    }
    std::cout << std::endl;
    
    // Test 4: TIMA increment speed(frequency 4.096 kHz = bit 9)
    std::cout << "Test 4: TIMA increment with TAC=0x04(4.096 kHz, bit 9)" << std::endl;
    Timer timer2;
    timer2.writeTMA(0xFA);  // Set TMA to 0xFA
    timer2.writeTIMA(0xFE);
    timer2.writeTAC(0x04);  // Enable timer, freq 4.096 kHz(bit 9)
    
    // Bit 9 transition: starts at 0, goes to 1 at 512, back to 0 at 1024
    // First falling edge is at 1024 cycles
    timer2.update(1024);
    std::cout << "After 1024 cycles: TIMA = 0x" << std::hex << (int)timer2.readTIMA() << std::dec;
    if(timer2.readTIMA() == 0xFF){
        std::cout << " ✓ (incremented once from 0xFE)" << std::endl;
    } else {
        std::cout << " (got 0x" << std::hex << (int)timer2.readTIMA() << std::dec << ")" << std::endl;
    }
    std::cout << std::endl;
    
    // Test 5: TIMA overflow and reload
    std::cout << "Test 5: TIMA overflow and TMA reload" << std::endl;
    Timer timer3;
    timer3.writeTMA(0x42);  // TMA = 0x42
    timer3.writeTIMA(0xFF);  // TIMA = 0xFF
    timer3.writeTAC(0x05);  // Enable timer, freq 262.144 kHz(bit 3)
    
    // With bit 3 selected, first falling edge is at 16 cycles(bit 3 goes 0->1 at 8, 1->0 at 16)
    timer3.update(16);  // One increment to overflow
    std::cout << "After 16 cycles: TIMA = 0x" << std::hex << (int)timer3.readTIMA() << std::dec;
    if(timer3.readTIMA() == 0x00 && timer3.hasInterrupt()){
        std::cout << " (overflow)" << std::endl;
        std::cout << "✓ TIMA overflowed and interrupt generated" << std::endl;
        std::cout << "TMA will reload: 0x" << std::hex << (int)timer3.readTMA() << std::dec << std::endl;
    } else {
        std::cout << std::endl;
        std::cout << "Interrupt: " << timer3.hasInterrupt() << std::endl;
    }
    std::cout << std::endl;
    
    // Test 6: Timer disabled
    std::cout << "Test 6: Timer with TAC enable bit = 0" << std::endl;
    Timer timer4;
    timer4.writeTIMA(0x00);
    timer4.writeTAC(0x00);  // Timer disabled
    timer4.update(512);
    std::cout << "After 512 cycles with timer disabled: TIMA = 0x" << std::hex << (int)timer4.readTIMA() << std::dec;
    if(timer4.readTIMA() == 0x00){
        std::cout << " ✓ (not incremented)" << std::endl;
    } else {
        std::cout << " ✗ (should be 0x00)" << std::endl;
    }
    std::cout << std::endl;
    
    // Test 7: TAC frequency change
    std::cout << "Test 7: Changing TAC frequency selection" << std::endl;
    Timer timer5;
    timer5.writeTIMA(0x00);
    timer5.writeTAC(0x04);  // Freq 4.096 kHz(bit 9)
    timer5.update(256);
    uint8_t tima_after_256 = timer5.readTIMA();
    std::cout << "TIMA after 256 cycles @ 4.096kHz: 0x" << std::hex << (int)tima_after_256 << std::dec << std::endl;
    
    timer5.writeTAC(0x05);  // Change to 262.144 kHz(bit 3)
    timer5.update(8);
    std::cout << "TIMA after 8 more cycles @ 262.144kHz: 0x" << std::hex << (int)timer5.readTIMA() << std::dec;
    if(timer5.readTIMA() > tima_after_256){
        std::cout << " ✓ (faster frequency)" << std::endl;
    }
    std::cout << std::endl;
    
    std::cout << "=== Timer Tests Complete ===" << std::endl;
    return 0;
}
