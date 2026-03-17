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
- C++17 compatible compiler (g++ 7.0+)
- Linux/WSL environment

### Compilation
```bash
g++ -std=c++17 main.cpp CPU.cpp Cartridge.cpp MMU.cpp PPU.cpp Timer.cpp Joypad.cpp -o emulator
```

With full error checking:
```bash
g++ -g -std=c++17 -Wall -Wextra -pedantic-errors -Weffc++ -Wno-unused-parameter -fsanitize=undefined,address main.cpp CPU.cpp Cartridge.cpp MMU.cpp PPU.cpp Timer.cpp Joypad.cpp -o emulator
```

### Running
```bash
./emulator <rom_file.gb>
```

Example:
```bash
./emulator Tetris.gb
```

## Testing

Test suites are provided for individual components:

```bash
# Test CPU opcodes
g++ -std=c++17 test_opcodes.cpp CPU.cpp Cartridge.cpp MMU.cpp -o test_opcodes
./test_opcodes

# Test CB-prefixed opcodes
g++ -std=c++17 test_cb_opcodes.cpp CPU.cpp Cartridge.cpp MMU.cpp -o test_cb_opcodes
./test_cb_opcodes

# Test interrupt system
g++ -std=c++17 test_interrupts.cpp CPU.cpp Cartridge.cpp MMU.cpp -o test_interrupts
./test_interrupts

# Test timer
g++ -std=c++17 test_timer.cpp Timer.cpp -o test_timer
./test_timer

# Test joypad
g++ -std=c++17 test_joypad.cpp Joypad.cpp -o test_joypad
./test_joypad

# Test ROM loading
g++ -std=c++17 test_rom_loading.cpp MMU.cpp Cartridge.cpp -o test_rom_loading
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

## Contributing
Contributions welcome! Areas with most potential impact:
- Graphics/PPU completion
- Additional cartridge type support
- Audio implementation
- Bug fixes and optimizations

## License
MIT License - See [LICENSE](LICENSE) file for details

This project is free to use, modify, and distribute. See the LICENSE file for full terms.

## References
- [Pan Docs](https://gbdev.io/pandocs/) - Game Boy technical reference
- [Opcode reference](https://www.pastraiser.com/cpu/gameboy/gameboy_opcodes.html)
- [Hardware specifications](https://github.com/gbdev/hardware.json)
