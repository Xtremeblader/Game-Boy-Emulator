#include "Joypad.h"
#include <iostream>
#include <iomanip>

int main(){
    std::cout << "=== Game Boy Joypad Tests ===" << std::endl << std::endl;
    
    // Test 1: Initialization
    std::cout << "Test 1: Joypad initialization" << std::endl;
    Joypad joypad;
    joypad.write(0xEF);  // Select D-Pad (bit 4 = 0)
    uint8_t initial = joypad.read();
    std::cout << "Initial state(all released): 0x" << std::hex << (int)initial << std::dec;
    if((initial & 0x0F) == 0x0F){
        std::cout << " ✓" << std::endl;
    } else {
        std::cout << " ✗ (expected bits 3-0 = 0x0F)" << std::endl;
    }
    std::cout << std::endl;
    
    // Test 2: D-Pad button press
    std::cout << "Test 2: D-Pad button press(RIGHT)" << std::endl;
    joypad.press(Joypad::RIGHT);
    joypad.write(0xEF);  // Select D-Pad
    uint8_t after_press = joypad.read();
    std::cout << "After pressing RIGHT: 0x" << std::hex << (int)after_press << std::dec;
    if((after_press & 0x01) == 0){
        std::cout << " ✓" << std::endl;
    } else {
        std::cout << " ✗" << std::endl;
    }
    std::cout << std::endl;
    
    // Test 3: Multiple D-Pad presses
    std::cout << "Test 3: Multiple D-Pad presses(LEFT and UP)" << std::endl;
    joypad.press(Joypad::LEFT);
    joypad.press(Joypad::UP);
    joypad.write(0xEF);  // Select D-Pad
    uint8_t multi_press = joypad.read();
    std::cout << "After pressing RIGHT, LEFT, UP: 0x" << std::hex << (int)multi_press << std::dec;
    // RIGHT = bit 0, LEFT = bit 1, UP = bit 2 -> 0x07
    if((multi_press & 0x07) == 0x00){
        std::cout << " ✓" << std::endl;
    } else {
        std::cout << " ✗ (expected bits 0-2 to be 0, got 0x" << std::hex << (int)(multi_press & 0x07) << std::dec << ")" << std::endl;
    }
    std::cout << std::endl;
    
    // Test 4: Release button
    std::cout << "Test 4: Release button(RIGHT)" << std::endl;
    joypad.release(Joypad::RIGHT);
    joypad.write(0xEF);  // Select D-Pad
    uint8_t after_release = joypad.read();
    std::cout << "After releasing RIGHT: 0x" << std::hex << (int)after_release << std::dec;
    // LEFT and UP still pressed = bits 1,2 = 0, RIGHT released = bit 0 = 1
    if((after_release & 0x01) == 1 && (after_release & 0x06) == 0){
        std::cout << " ✓" << std::endl;
    } else {
        std::cout << " ✗" << std::endl;
    }
    std::cout << std::endl;
    
    // Test 5: Action buttons
    std::cout << "Test 5: Action button press(A)" << std::endl;
    Joypad joypad2;
    joypad2.press(Joypad::A);
    joypad2.write(0xDF);  // Select buttons (bit 5 = 0)
    uint8_t button_state = joypad2.read();
    std::cout << "After pressing A(button select): 0x" << std::hex << (int)button_state << std::dec;
    // A is pressed, so bit 0 reads zero.
    if((button_state & 0x0F) == 0x0E){
        std::cout << " ✓" << std::endl;
    } else {
        std::cout << " (bits 3-0: 0x" << std::hex << (int)(button_state & 0x0F) << std::dec << ")" << std::endl;
    }
    std::cout << std::endl;
    
    // Test 6: Button isolation
    std::cout << "Test 6: Button isolation(selecting D-Pad hides buttons)" << std::endl;
    joypad2.press(Joypad::LEFT);    // D-Pad press
    joypad2.press(Joypad::START);   // Button press
    joypad2.write(0xEF);  // Select only D-Pad (bit 4 = 0)
    uint8_t dpad_only = joypad2.read();
    joypad2.write(0xDF);  // Select only buttons (bit 5 = 0)
    uint8_t buttons_only = joypad2.read();
    
    std::cout << "D-Pad selected: 0x" << std::hex << (int)dpad_only << std::dec;
    if((dpad_only & 0x02) == 0){  // LEFT should be pressed(bit 1)
        std::cout << " ✓ (shows D-Pad)" << std::endl;
    }
    std::cout << "Buttons selected: 0x" << std::hex << (int)buttons_only << std::dec;
    if((buttons_only & 0x08) == 0){  // START should be pressed(bit 3)
        std::cout << " ✓ (shows buttons)" << std::endl;
    }
    std::cout << std::endl;
    
    // Test 7: Interrupt generation
    std::cout << "Test 7: Interrupt generation on button press" << std::endl;
    Joypad joypad3;
    if(!joypad3.hasInterrupt()){
        std::cout << "✓ No interrupt initially" << std::endl;
    }
    joypad3.press(Joypad::A);
    if(joypad3.hasInterrupt()){
        std::cout << "✓ Interrupt generated on press" << std::endl;
    } else {
        std::cout << "✗ Interrupt should be generated" << std::endl;
    }
    joypad3.clearInterrupt();
    if(!joypad3.hasInterrupt()){
        std::cout << "✓ Interrupt cleared" << std::endl;
    }
    std::cout << std::endl;
    
    std::cout << "=== Joypad Tests Complete ===" << std::endl;
    return 0;
}
