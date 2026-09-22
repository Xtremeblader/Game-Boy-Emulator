#pragma once
#include <cstdint>

class Joypad {
public:
    Joypad();
    
    // Button enumeration
    enum Button {
        // D-Pad buttons
        RIGHT = 0,
        LEFT = 1,
        UP = 2,
        DOWN = 3,
        // Action buttons
        A = 4,
        B = 5,
        SELECT = 6,
        START = 7
    };
    
    // Update button state
    void press(Button button);
    void release(Button button);
    void setButtonState(Button button, bool pressed);
    
    // JOYPAD register(0xFF00) read/write
    uint8_t read() const;
    void write(uint8_t value);
    
    // Check if any button changed(for interrupt generation)
    bool hasInterrupt() const { return interrupt_triggered; }
    void clearInterrupt(){ interrupt_triggered = false; }
    
private:
    // Separate state for D-Pad(bits 0-3) and buttons(bits 0-3)
    uint8_t dpad_state;     // Bits 0-3: Right, Left, Up, Down
    uint8_t button_state;   // Bits 0-3: A, B, Select, Start
    
    // JOYPAD register state
    uint8_t joypad_register;  // Bit 5 = Button select, Bit 4 = D-Pad select
    
    // Interrupt flag
    bool interrupt_triggered;
};
