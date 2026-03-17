#include "Timer.h"

Timer::Timer()
    : internal_counter(0),
      tima(0),
      tma(0),
      tac(0),
      prev_edge_detector(false),
      timer_interrupt(false){
}

void Timer::update(int cycles){
    // Check if timer is enabled(TAC bit 2)
    bool timer_enabled = (tac & 0x04) != 0;
    
    if(!timer_enabled){
        // Increment internal counter but don't check for TIMA update
        internal_counter += cycles;
        return;
    }
    
    // For each cycle, check the selected bit for a falling edge
    for(int i = 0; i < cycles; i++){
        uint16_t bit_pos = getTimerBit();
        uint8_t prev_bit = (internal_counter >> bit_pos) & 1;
        
        // Increment counter
        internal_counter++;
        
        // Check for falling edge(1 to 0 transition)
        uint8_t curr_bit = (internal_counter >> bit_pos) & 1;
        
        if(prev_bit == 1 && curr_bit == 0){
            // Falling edge detected - increment TIMA
            tima++;
            
            // Check for overflow  
            if(tima == 0){
                tima = tma;
                timer_interrupt = true;
            }
        }
    }
}

uint16_t Timer::getTimerBit() const {
    // Select which bit of the 16-bit counter to monitor based on TAC bits 0-1
    uint8_t frequency = tac & 0x03;
    
    switch(frequency){
        case 0: return 9;   // 4.096 kHz
        case 1: return 3;   // 262.144 kHz
        case 2: return 5;   // 65.536 kHz
        case 3: return 7;   // 16.384 kHz
        default: return 9;
    }
}

void Timer::writeTAC(uint8_t value){
    // Check for glitch when changing TAC
    // If the selected bit falls from 1 to 0, increment TIMA
    
    uint16_t prev_bit_pos = (tac & 0x03) == 0 ? 9 :
                             (tac & 0x03) == 1 ? 3 :
                             (tac & 0x03) == 2 ? 5 : 7;
    
    uint16_t new_bit_pos = (value & 0x03) == 0 ? 9 :
                           (value & 0x03) == 1 ? 3 :
                           (value & 0x03) == 2 ? 5 : 7;
    
    bool prev_enabled = (tac & 0x04) != 0;
    bool new_enabled = (value & 0x04) != 0;
    
    // Check for falling edge on old bit with old enable state
    if(prev_enabled && ((internal_counter >> prev_bit_pos) & 1)){
        // Old bit was 1 and timer was enabled
        // Check if it will become 0 with new settings
        bool old_bit_will_be_zero = !(((internal_counter >> new_bit_pos) & 1) && new_enabled);
        
        if(old_bit_will_be_zero){
            // Falling edge occurred
            tima++;
            if(tima == 0){
                tima = tma;
                timer_interrupt = true;
            }
        }
    }
    
    tac = value;
}

