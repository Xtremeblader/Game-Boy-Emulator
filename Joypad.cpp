#include "Joypad.h"

Joypad::Joypad() 
    : dpad_state(0x0F),      // All D-Pad released
      button_state(0x0F),    // All buttons released  
      joypad_register(0xCF),
      interrupt_triggered(false){
}

void Joypad::press(Button button){
    setButtonState(button, true);
}

void Joypad::release(Button button){
    setButtonState(button, false);
}

void Joypad::setButtonState(Button button, bool pressed){
    uint8_t* target_state;
    uint8_t previous_value;
    
    if(button < 4){
        // D-Pad button(0-3)
        target_state = &dpad_state;
    } else {
        // Action button(4-7) - map to 0-3
        button = static_cast<Button>(button - 4);
        target_state = &button_state;
    }
    
    previous_value = *target_state;
    
    if(pressed){
        // Press: clear the bit(0 = pressed)
        *target_state &= ~(1 << button);
    } else {
        // Release: set the bit(1 = released)
        *target_state |= (1 << button);
    }
    
    // Generate interrupt if state changed(only on press)
    if(pressed && ((previous_value & (1 << button)) != 0)){
        interrupt_triggered = true;
    }
}

uint8_t Joypad::read(){
    // Reading JOYPAD register(0xFF00)
    // Bits 7-6: Unused(always 1)
    // Bit 5: P15 - D-Pad row select(0 = select D-Pad)
    // Bit 4: P14 - Button row select(0 = select buttons)
    // Bits 3-0: D3-D0 - Selected input state
    
    uint8_t result = joypad_register & 0xF0;  // Keep bits 4-7
    
    if((joypad_register & 0x20) == 0){
        // D-Pad is selected(bit 5 = 0)
        result |= dpad_state;
    } else if((joypad_register & 0x10) == 0){
        // Buttons are selected(bit 4 = 0)
        result |= button_state;
    } else {
        // Neither selected(both bits set)
        result |= 0x0F;
    }
    
    return result;
}

void Joypad::write(uint8_t value){
    // Writing to JOYPAD register selects which buttons to read
    // Only bits 4-5 are used for selection
    joypad_register = (value & 0xF0) | 0x0F;
}

