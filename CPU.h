#pragma once
#include <cstdint>
#include "MMU.h"

union Register {
    uint16_t reg;
    struct {
        uint8_t lo;
        uint8_t hi;
    } bytes;
};

class CPU {
public:
    CPU(MMU& mmu_ref);
    int step();
    
    // Public for testing
    Register AF;
    Register BC;
    Register DE;
    Register HL;
    uint16_t PC;
    uint16_t SP;
    
    enum FlagBit {
        ZERO = 7,
        SUBTRACT = 6,
        HALF_CARRY = 5,
        CARRY = 4
    };
    
    bool getFlag(FlagBit flag) const {
        return(AF.bytes.lo & (1 << flag)) != 0;
    }
    
    // Interrupt registers(public for testing)
    uint8_t IE;  // Interrupt Enable(0xFFFF): bits 0-4 enable V-Blank, LCD STAT, Timer, Serial, Joypad
    uint8_t IF;  // Interrupt Flags(0xFF0F): bits 0-4 signal pending interrupts
    bool IME;    // Interrupt Master Enable flag(public for testing)
    
    enum InterruptType {
        V_BLANK = 0,
        LCD_STAT = 1,
        TIMER = 2,
        SERIAL = 3,
        JOYPAD = 4
    };
    
    const uint16_t INTERRUPT_VECTORS[5] = {
        0x0040,  // V-Blank
        0x0048,  // LCD STAT
        0x0050,  // Timer
        0x0058,  // Serial
        0x0060   // Joypad
    };

private:
    MMU& mmu;
    
    uint8_t ime_scheduled;  // Instructions remaining before EI takes effect
    bool halted = false;
    bool halt_bug = false;
    
    uint8_t fetchByte();
    uint16_t fetchWord();
    int execute(uint8_t opcode);
    
    // Interrupt handling
    int checkInterrupts();
    void serviceInterrupt(InterruptType type);

    void setFlag(FlagBit flag){
        AF.bytes.lo |= (1 << flag);
    }

    void clearFlag(FlagBit flag){
        AF.bytes.lo &= ~(1 << flag);
    }

    // 8-bit Arithmetic Operations
    void add8(uint8_t& reg, uint8_t value);
    void adc8(uint8_t& reg, uint8_t value);     // Add with carry
    void sub8(uint8_t& reg, uint8_t value);
    void sbc8(uint8_t& reg, uint8_t value);     // Subtract with carry
    void inc8(uint8_t& reg);
    void dec8(uint8_t& reg);

    // 16-bit Arithmetic Operations
    void add16(uint16_t& reg_pair, uint16_t value);
    void inc16(uint16_t& reg_pair);
    void dec16(uint16_t& reg_pair);

    // Bitwise Operations
    void and8(uint8_t& reg, uint8_t value);
    void and8bit(uint8_t value);                // Legacy - operates on A
    void or8(uint8_t& reg, uint8_t value);
    void xor8(uint8_t& reg, uint8_t value);
    void compare(uint8_t value);                // Legacy - compares with A

    // Rotation Operations
    void rlc(uint8_t& reg);                     // Rotate left circular
    void rrc(uint8_t& reg);                     // Rotate right circular
    void rl(uint8_t& reg);                      // Rotate left through carry
    void rr(uint8_t& reg);                      // Rotate right through carry

    // Shift Operations
    void sla(uint8_t& reg);                     // Shift left arithmetic
    void sra(uint8_t& reg);                     // Shift right arithmetic
    void srl(uint8_t& reg);                     // Shift right logical
    void swap(uint8_t& reg);                    // Swap nibbles

    // Bit Operations
    void bit(uint8_t value, int bit_pos);       // Test bit(sets ZERO flag)
    void set(uint8_t& value, int bit_pos);      // Set bit
    void res(uint8_t& value, int bit_pos);      // Reset bit

    // A-specific Rotations(different flag behavior than CB versions)
    void rlca();                                 // Rotate A left circular(clears Z)
    void rrca();                                 // Rotate A right circular(clears Z)
    void rla();                                  // Rotate A left through carry(clears Z)
    void rra();                                  // Rotate A right through carry(clears Z)

    // Flag Operations
    void cpl();                                  // Complement A(flip all bits)
    void scf();                                  // Set carry flag
    void ccf();                                  // Complement carry flag
    void daa();                                  // Decimal adjust A(after BCD arithmetic)

    // Stack Operations
    void pushWord(uint16_t value);               // Push 16-bit value to stack
    uint16_t popWord();                         // Pop 16-bit value from stack

    // Interrupt control
    void enableInterrupts();
    void disableInterrupts();

    // CB-prefixed instruction handler
    int executeCB(uint8_t opcode);
};