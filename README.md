# Game Boy Emulator

A C++ implementation of a Game Boy emulator, building a cycle-accurate CPU emulator with support for Game Boy hardware components.

## Project Status

### Completed ✓
- **CPU Core**
  - Z80-compatible instruction set (all 256 main opcodes)
  - CB-prefixed opcodes (all 256 extended instructions)
  - Full flag system (Zero, Subtract, Half-Carry, Carry)
  - Interrupt handling (V-Blank, LCD STAT, Timer, Serial, Joypad)
  - EI/DI interrupt control with proper 1-cycle delay
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
  - Battery backup support detection

- **Peripherals**
  - **Timer**: TAC/TIMA/TMA registers with falling-edge detection
  - **Joypad**: Button input interface (D-Pad and action buttons)
  - **PPU** (Partial): 
    - LCD control registers (LCDC, STAT, etc.)
    - Scanline rendering pipeline (OAM search, pixel transfer, H-Blank, V-Blank)
    - Background tile rendering
    - Palette support (BGP, OBP0, OBP1)

### In Progress 🔄
- Graphics rendering (sprites, window layer)
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

The window displays the 160×144 background at 4× size initially. Resize the
window as needed; Escape or closing the window exits. Emulation is paced to
approximately 59.73 host frames per second. Keyboard gameplay input is not yet
connected. Window-layer graphics, sprites, audio, and full game compatibility
remain unfinished; a visible background does not imply a fully playable game.
The Tetris startup smoke test now reaches the title screen. The unconnected
joypad reports all buttons released to avoid triggering the game's soft reset.
Keyboard input is still needed to start playing. Use `--test-pattern` to verify
the graphics path independently.

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

### Graphics implementation
The CPU's elapsed clock cycles advance the PPU; MMU graphics register accesses
are forwarded to it. VBlank and STAT requests set the CPU interrupt flags.
The background renderer supports scrolling, both tile maps, signed/unsigned
tile addressing, and the DMG background palette. Timing currently uses fixed
80/172/204-dot visible-line phases and ten VBlank lines; variable pixel-transfer
timing and CPU VRAM/OAM access restrictions are not yet implemented.

## Testing

Run the graphics regression suite with `make test`. It checks tile decoding,
register mapping, scanline/frame timing, LCD disable behavior, and STAT edges.
It also checks nested CALL/RET, taken and untaken conditional returns, RETI,
and released joypad reads.

Test suites are also provided for individual components:

```bash
# Test CPU opcodes
g++ -std=c++17 test_opcodes.cpp CPU.cpp Cartridge.cpp MMU.cpp PPU.cpp -o test_opcodes
./test_opcodes

# Test CB-prefixed opcodes
g++ -std=c++17 test_cb_opcodes.cpp CPU.cpp Cartridge.cpp MMU.cpp PPU.cpp -o test_cb_opcodes
./test_cb_opcodes

# Test interrupt system
g++ -std=c++17 test_interrupts.cpp CPU.cpp Cartridge.cpp MMU.cpp PPU.cpp -o test_interrupts
./test_interrupts

# Test timer
g++ -std=c++17 test_timer.cpp Timer.cpp -o test_timer
./test_timer

# Test joypad
g++ -std=c++17 test_joypad.cpp Joypad.cpp -o test_joypad
./test_joypad

# Test ROM loading
g++ -std=c++17 test_rom_loading.cpp MMU.cpp Cartridge.cpp PPU.cpp -o test_rom_loading
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
- Proper EI instruction 1-cycle delay

## Known Limitations
- Graphics rendering incomplete (background rendering only)
- No sprite support yet
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
