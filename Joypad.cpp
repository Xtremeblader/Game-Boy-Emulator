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
    if(button < RIGHT || button > START) return;
    uint8_t previous = read();
    uint8_t& state = button < 4 ? dpad_state : button_state;
    int bit = static_cast<int>(button) % 4;
    if(pressed) state &= ~(1 << bit);
    else state |= 1 << bit;
    if(previous & ~read() & 0x0F) interrupt_triggered = true;
}

uint8_t Joypad::read() const {
    uint8_t inputs = 0x0F;
    if(!(joypad_register & 0x10)) inputs &= dpad_state;
    if(!(joypad_register & 0x20)) inputs &= button_state;
    return 0xC0 | (joypad_register & 0x30) | inputs;
}

void Joypad::write(uint8_t value){
    uint8_t previous = read();
    joypad_register = value & 0x30;
    // Selecting a row with a held key can also produce a falling input edge.
    if(previous & ~read() & 0x0F) interrupt_triggered = true;
}
