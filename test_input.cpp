#include "CPU.h"
#include "Joypad.h"
#include <cassert>
#include <iostream>
#ifdef GB_WITH_SDL
#include "Display.h"
#include <SDL3/SDL.h>
#endif

int main(){
    MMU mmu;
    CPU cpu(mmu);
    Joypad pad;
    mmu.linkJoypad(&pad);
    for(int i = 0; i < 8; ++i){
        auto button = static_cast<Joypad::Button>(i);
        uint8_t row = i < 4 ? 0x20 : 0x10;
        mmu.writeByte(0xFF00, row);
        cpu.IF = 1;
        pad.press(button);
        mmu.syncJoypadInterrupt();
        assert(mmu.readByte(0xFF00) == ((0xCF | row) & ~(1 << (i % 4))));
        assert(cpu.IF == 0x11); // Preserve other pending interrupts.
        cpu.IF = 0;
        pad.press(button);
        mmu.syncJoypadInterrupt();
        assert(cpu.IF == 0); // No repeated interrupt for a held key.
        pad.release(button);
        mmu.syncJoypadInterrupt();
        assert(cpu.IF == 0 && (mmu.readByte(0xFF00) & 15) == 15);
    }
    mmu.writeByte(0xFF00, 0x30);
    pad.press(Joypad::A);
    mmu.syncJoypadInterrupt();
    assert(cpu.IF == 0 && mmu.readByte(0xFF00) == 0xFF);
    mmu.writeByte(0xFF00, 0x10);
    assert(cpu.IF == 0x10 && mmu.readByte(0xFF00) == 0xDE);
    pad.press(Joypad::LEFT);
    mmu.writeByte(0xFF00, 0); // Both rows: combine active-low inputs.
    assert(mmu.readByte(0xFF00) == 0xCC);
    pad.release(Joypad::A);
    pad.release(Joypad::LEFT);
    mmu.writeByte(0xFF00, 0xFF);
    assert(mmu.readByte(0xFF00) == 0xFF); // Only row bits are writable.

    // A selected input edge reaches the actual CPU joypad vector.
    cpu.IF = 0;
    cpu.IE = 0x10;
    cpu.IME = true;
    cpu.PC = 0xC000;
    cpu.SP = 0xD000;
    mmu.writeByte(0xFF00, 0x10);
    pad.press(Joypad::START);
    mmu.syncJoypadInterrupt();
    assert(cpu.step() == 20 && cpu.PC == 0x60 && cpu.IF == 0);
    assert(mmu.readWord(cpu.SP) == 0xC000);
    pad.release(Joypad::START);

#ifdef GB_WITH_SDL
    Display display;
    assert(display.open());
    assert(display.poll(pad));
    const SDL_Scancode keys[] = {SDL_SCANCODE_RIGHT, SDL_SCANCODE_LEFT,
        SDL_SCANCODE_UP, SDL_SCANCODE_DOWN, SDL_SCANCODE_Z, SDL_SCANCODE_X,
        SDL_SCANCODE_BACKSPACE, SDL_SCANCODE_RETURN};
    auto key = [](SDL_Scancode code, bool down, bool repeat = false){
        SDL_Event event{};
        event.type = down ? SDL_EVENT_KEY_DOWN : SDL_EVENT_KEY_UP;
        event.key.scancode = code;
        event.key.down = down;
        event.key.repeat = repeat;
        assert(SDL_PushEvent(&event));
    };
    mmu.writeByte(0xFF00, 0);
    for(int i = 0; i < 8; ++i){
        key(keys[i], true);
        assert(display.poll(pad));
        assert((pad.read() & 15) == (15 & ~(1 << (i % 4))));
        pad.clearInterrupt();
        key(keys[i], true, true);
        assert(display.poll(pad) && !pad.hasInterrupt());
        key(keys[i], false);
        assert(display.poll(pad) && (pad.read() & 15) == 15);
    }
    key(SDL_SCANCODE_RIGHT, true);
    key(SDL_SCANCODE_X, true);
    assert(display.poll(pad) && (pad.read() & 15) == 12);
    SDL_Event focus{};
    focus.type = SDL_EVENT_WINDOW_FOCUS_LOST;
    assert(SDL_PushEvent(&focus));
    assert(display.poll(pad) && (pad.read() & 15) == 15);
    SDL_Event quit{};
    quit.type = SDL_EVENT_QUIT;
    assert(SDL_PushEvent(&quit));
    assert(!display.poll(pad));
#endif
    std::cout << "Joypad register, interrupt, and input tests passed\n";
}
