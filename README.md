# Game Boy Emulator

A C++ implementation of a Game Boy emulator, building a cycle-accurate CPU emulator with support for Game Boy hardware components.

## Project Status

### Completed ✓
- **CPU Core**
  - Z80-compatible instruction set (all 256 main opcodes)
  - CB-prefixed opcodes (all 256 extended instructions)
  - Full flag system (Zero, Subtract, Half-Carry, Carry)
  - Interrupt handling (V-Blank, LCD STAT, Timer, Serial, Joypad)
  - EI/DI interrupt control with a one-instruction EI delay
  - Stack operations (PUSH/POP)
  - All arithmetic operations (ADD, SUB, INC, DEC, etc.)
  - All bitwise operations (AND, OR, XOR, bit ops)
  - All shift/rotate operations (RLC, RRC, RL, RR, SLA, SRA, SRL)

- **Memory Management (MMU)**
  - Complete memory map implementation (0x0000-0xFFFF)
  - ROM area (0x0000-0x7FFF)
  - VRAM (0x8000-0x9FFF)
  - Work RAM (0xC000-0xDFFF) with echo (0xE000-0xFDFF)
  - External RAM (0xA000-0xBFFF)
  - OAM (0xFE00-0xFE9F)
  - I/O Registers (0xFF00-0xFF7F)
  - High RAM (0xFF80-0xFFFF)

- **Cartridge Support**
  - ROM-only cartridges
  - MBC1 (Memory Bank Controller 1)
  - MBC3 (with RTC support planned)
  - MBC5
  - Header parsing and validation
  - ROM and RAM banking
  - Battery-backed RAM loading, periodic saves, and shutdown saves

- **Peripherals**
  - **Timer**: TAC/TIMA/TMA registers with falling-edge detection
  - **Joypad**: Button input interface (D-Pad and action buttons)
  - **PPU** (Partial): 
    - LCD control registers (LCDC, STAT, etc.)
    - Scanline rendering pipeline (OAM search, pixel transfer, H-Blank, V-Blank)
    - Background and window tile rendering
    - 8×8/8×16 sprites, palettes, flips, and DMG priority
    - OAM DMA transfers (one byte per four CPU clocks)
    - Palette support (BGP, OBP0, OBP1)

### In Progress 🔄
- Graphics timing accuracy and memory-access restrictions
- Serial communication interface
- Real-time clock (RTC) in MBC3 cartridges

### Planned 📋
- Sound/Audio (APU)
- HDMA transfers
- Double-speed mode (CGB)
- Full game compatibility testing

## Building

### Requirements
- C++17 compiler and Make
- Linux/WSL; a working graphical session (such as WSLg) for the window
- SDL3 development files and pkg-config for the graphical build

On Ubuntu versions that provide SDL3:
```bash
sudo apt-get install build-essential libsdl3-dev pkg-config
make
./emulator 'Tetris (JUE) (V1.1) [!].gb'
```

If your distribution does not package SDL3, install it using the
[official SDL instructions](https://wiki.libsdl.org/SDL3/Installation).
The display uses the SDL3 API, not SDL2.

The window displays the 160×144 framebuffer at 4× size initially. Resize the
window as needed; Escape or closing the window exits. Emulation is paced to
approximately 59.73 host frames per second. Keyboard input is connected. Click the emulator window to give it focus.

| Keyboard | Game Boy |
| --- | --- |
| Arrow keys | D-pad |
| Z | A |
| X | B |
| Enter | Start |
| Backspace | Select |
| Escape | Close emulator |

Z/X use physical key positions. Keys stay pressed until released; switching
away from the window releases all buttons. Audio and full game compatibility
remain unfinished. Tetris and Pokémon Red reach their title screens; press
Enter to advance, then use Z to confirm choices. Full gameplay has not been verified. Battery-backed cartridge RAM is saved
automatically as described below.

### Graphics smoke test (no ROM needed)
```bash
./emulator --test-pattern
```
Expect repeating white, light-gray, dark-gray, and black vertical stripes.

### Headless build and frame capture
No SDL dependency is required for this build:
```bash
make headless
./emulator-headless --headless --test-pattern --frames 1 --screenshot pattern.ppm
./emulator-headless --headless 'Tetris (JUE) (V1.1) [!].gb' --frames 120 --screenshot tetris.ppm
```

`--frames N` limits execution to N host intervals of 70,224 CPU clock cycles,
including when the emulated LCD is disabled. Headless mode defaults to 60 such
intervals. `--screenshot` saves the final framebuffer as a binary PPM image.
The normal graphical run continues until you close it. Use `--help` for options.

### Battery saves
Use the game's own SAVE command first (for example, SAVE in Pokémon's menu).
Battery-backed cartridge RAM is restored automatically on launch from a raw
`.sav` file next to the ROM: `Pokemon Red.gb` uses `Pokemon Red.sav`.

Changed RAM is written every 300 emulated frames (about five seconds), when
you close the window or press Escape, and on Ctrl+C/SIGTERM. Headless runs
also save when their frame limit is reached. These are cartridge saves, not
snapshots of the CPU or the current screen. Games without battery-backed RAM
(such as Tetris) do not create save files.

Saves contain all allocated RAM banks. Writes go to a unique temporary file
in the same directory, then replace the old save after a successful write and
flush. Save failures are reported; malformed or unreadable existing saves
stop ROM loading instead of being overwritten. The ROM directory must be
writable. Save files and temporary save files are ignored by Git.

RTC state and unsupported cartridge controllers (including MBC2) are not
persisted. Avoid running two instances with the same ROM/save path at once.
A forced kill can lose changes since the last periodic save.

### CPU HALT and interrupt timing
HALT pauses instruction fetches while the emulator continues advancing the
PPU, timer, and DMA in four-clock steps. Enabled pending interrupts wake the
CPU; IME determines whether it services the interrupt or resumes execution.
The DMG HALT bug and EI/HALT interaction are modeled. EI enables interrupts
after the next instruction, DI cancels a pending enable, and RETI enables
interrupts immediately. `make test` includes wakeup and peripheral-clock tests.
Instruction-internal interrupt sampling and STOP remain simplified.

### Timer integration
The CPU clock advances DIV and TIMA after each instruction. FF04–FF07 are
mapped to the timer, and overflow requests the timer interrupt in IF. DIV
continues counting when TIMA is disabled, so games can use it for randomness.
A scripted Tetris run reproduced only square pieces without the timer and all
seven piece types with it connected.

The timer models selectable frequencies, DIV/TAC falling-edge ticks, a
four-clock overflow reload delay, and cancellation by a TIMA write during that
delay. CPU bus accesses still happen at instruction boundaries; exact writes
during the reload cycle and STOP behavior are not yet modeled.

### Graphics implementation
The CPU's elapsed clock cycles advance the PPU; MMU graphics register accesses
are forwarded to it. VBlank and STAT requests set the CPU interrupt flags.
The background renderer supports scrolling, both tile maps, signed/unsigned
tile addressing, and the DMG background palette. The window uses its own tile
map and line counter. Sprites support both sizes, flipping, transparency,
palettes, the ten-per-line limit, and DMG overlap/background priority. Writes
to FF46 start a 160-byte OAM DMA transfer, advanced by CPU clock cycles.
Timing currently uses fixed
80/172/204-dot visible-line phases and ten VBlank lines; variable pixel-transfer
timing, exact window-trigger quirks, CPU VRAM/OAM access restrictions, and DMA
bus restrictions/startup timing are not yet implemented.

## Testing

Run the regression suites with `make test`. Use `make test-input-sdl` to
exercise real SDL keyboard events with the dummy video driver. Input tests
cover all buttons, row selection, simultaneous inputs, interrupts, key repeat,
and focus loss. It checks tile decoding,
register mapping, scanline/frame timing, LCD disable behavior, and STAT edges.
It also checks nested CALL/RET, taken and untaken conditional returns, RETI,
and released joypad reads. Pokémon regression tests exhaustively check CP
flags and exercise DMA timing/restarts, sprite transparency/priority/flipping,
and window positioning/line counting. Timer tests check DIV, all four clock
selections, register-write edges, delayed reload/cancellation, and CPU timer
interrupt delivery.

Test suites are also provided for individual components:

```bash
# Test CPU opcodes
g++ -std=c++17 test_opcodes.cpp CPU.cpp Cartridge.cpp MMU.cpp PPU.cpp Joypad.cpp Timer.cpp -o test_opcodes
./test_opcodes

# Test CB-prefixed opcodes
g++ -std=c++17 test_cb_opcodes.cpp CPU.cpp Cartridge.cpp MMU.cpp PPU.cpp Joypad.cpp Timer.cpp -o test_cb_opcodes
./test_cb_opcodes

# Test interrupt system
g++ -std=c++17 test_interrupts.cpp CPU.cpp Cartridge.cpp MMU.cpp PPU.cpp Joypad.cpp Timer.cpp -o test_interrupts
./test_interrupts

# Test timer
g++ -std=c++17 test_timer.cpp Timer.cpp -o test_timer
./test_timer

# Test joypad
g++ -std=c++17 test_joypad.cpp Joypad.cpp -o test_joypad
./test_joypad

# Test ROM loading
g++ -std=c++17 test_rom_loading.cpp MMU.cpp Cartridge.cpp PPU.cpp Joypad.cpp Timer.cpp -o test_rom_loading
./test_rom_loading
```

## Project Structure

```
gb_emulatorproj/
├── CPU.cpp/h          # Z80 CPU core with instruction set
├── MMU.cpp/h          # Memory management unit
├── Cartridge.cpp/h    # Cartridge/ROM loading and banking
├── PPU.cpp/h          # Graphics processor (partial)
├── Timer.cpp/h        # Timer hardware
├── Joypad.cpp/h       # Input handling
├── main.cpp           # Emulator entry point
├── test_*.cpp         # Test suites for each component
└── README.md          # This file
```

## Architecture Notes

### CPU Implementation
- Full Z80 instruction decode and execution
- Proper flag handling for all arithmetic operations
- Support for all addressing modes
- Cycle-accurate timing

### Memory Model
- Linear 16-bit address space
- Proper memory banking for cartridge support
- I/O register mapping for hardware peripherals

### Interrupt System
- Five interrupt types with priority handling
- Interrupt Master Enable (IME) flag
- EI takes effect after the following instruction completes

## Known Limitations
- Graphics timing and bus-access restrictions are simplified
- No audio/sound
- Limited game compatibility (mainly CPU-focused)
- No save state support

## Future Improvements
- Complete graphics pipeline
- Serial port emulation
- Save/load game state
- Debug interface/debugger
- Performance optimization
- Support for Game Boy Color (CGB) extensions

## License
MIT License - See [LICENSE](LICENSE) file for details

This project is free to use, modify, and distribute. See the LICENSE file for full terms.

## References
- [Pan Docs](https://gbdev.io/pandocs/) - Game Boy technical reference
- [Opcode reference](https://www.pastraiser.com/cpu/gameboy/gameboy_opcodes.html)
- [Hardware specifications](https://github.com/gbdev/hardware.json)
