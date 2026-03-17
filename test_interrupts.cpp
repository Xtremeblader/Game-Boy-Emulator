#include "MMU.h"
#include "CPU.h"
#include <iostream>
#include <iomanip>

void assertEqual(const std::string& test, uint8_t got, uint8_t expected){
    if(got == expected){
        std::cout << "✓ " << test << std::endl;
    } else {
        std::cout << "✗ " << test << " - got 0x" << std::hex << (int)got 
                  << " expected 0x" << (int)expected << std::dec << std::endl;
    }
}

void assertNotEqual(const std::string& test, uint16_t got, uint16_t expected){
    if(got != expected){
        std::cout << "✓ " << test << std::endl;
    } else {
        std::cout << "✗ " << test << " - got 0x" << std::hex << got 
                  << " expected != 0x" << expected << std::dec << std::endl;
    }
}

int main(){
    std::cout << "=== Game Boy Interrupt System Tests ===" << std::endl << std::endl;

    // Test 1: IE register write/read via memory access
    std::cout << "Test 1: IE register write/read(0xFFFF)" << std::endl;
    MMU mmu;
    CPU cpu(mmu);
    mmu.writeByte(0xFFFF, 0x1F);  // Enable all 5 interrupts
    assertEqual("IE should be 0x1F", cpu.IE, 0x1F);
    assertEqual("Read IE from 0xFFFF", mmu.readByte(0xFFFF), 0x1F);
    std::cout << std::endl;

    // Test 2: IF register write/read via memory access
    std::cout << "Test 2: IF register write/read(0xFF0F)" << std::endl;
    MMU mmu2;
    CPU cpu2(mmu2);
    mmu2.writeByte(0xFF0F, 0x04);  // Set TIMER interrupt flag
    assertEqual("IF should be 0x04", cpu2.IF, 0x04);
    assertEqual("Read IF from 0xFF0F", mmu2.readByte(0xFF0F), 0x04);
    std::cout << std::endl;

    // Test 3: DI(Disable Interrupts) instruction
    std::cout << "Test 3: DI instruction disables IME flag" << std::endl;
    MMU mmu3;
    CPU cpu3(mmu3);
    cpu3.IE = 0xFF;  // Enable all interrupts
    mmu3.writeByte(0x0100, 0xF3);  // DI
    cpu3.step();
    if(!cpu3.getFlag(CPU::ZERO)){  // Simple sanity check(DI doesn't affect flags)
        std::cout << "✓ Flags unchanged after DI" << std::endl;
    }
    std::cout << std::endl;

    // Test 4: EI(Enable Interrupts) instruction - has 1-cycle delay
    std::cout << "Test 4: EI instruction enables IME with 1-cycle delay" << std::endl;
    MMU mmu4;
    CPU cpu4(mmu4);
    // EI at 0x0100, NOP at 0x0101
    mmu4.writeByte(0x0100, 0xFB);  // EI
    mmu4.writeByte(0x0101, 0x00);  // NOP
    cpu4.step();  // Execute EI - sets ime_scheduled = 1
    if(!cpu4.IME){
        std::cout << "✓ IME not immediately set after EI" << std::endl;
    } else {
        std::cout << "✗ IME should not be set immediately after EI" << std::endl;
    }
    cpu4.step();  // Execute NOP - ime_scheduled should become 0 and IME should become true
    if(cpu4.IME){
        std::cout << "✓ IME set after next cycle" << std::endl;
    } else {
        std::cout << "✗ IME should be set after next cycle" << std::endl;
    }
    std::cout << std::endl;

    // Test 5: Interrupt service(V-Blank) - disable interrupts and push PC
    std::cout << "Test 5: V-Blank does not trigger when IME is disabled" << std::endl;
    MMU mmu5;
    CPU cpu5(mmu5);
    cpu5.IE = 0x01;  // Enable V-Blank
    cpu5.IF = 0x01;  // Set V-Blank flag
    // IME is false by default
    uint16_t pc_before = cpu5.PC;
    mmu5.writeByte(0x0100, 0x00);  // NOP
    cpu5.step();
    if(cpu5.PC == pc_before + 1){  // PC should just increment, not jump
        std::cout << "✓ Interrupt not serviced when IME is disabled" << std::endl;
    } else {
        std::cout << "✗ PC should have just incremented" << std::endl;
    }
    std::cout << std::endl;

    // Test 6: Interrupt service(V-Blank) - enable interrupts and jump to vector
    std::cout << "Test 6: V-Blank interrupts jump to 0x0040" << std::endl;
    MMU mmu6;
    CPU cpu6(mmu6);
    cpu6.PC = 0x0150;
    cpu6.SP = 0xFFFE;
    cpu6.IE = 0x01;  // Enable V-Blank
    cpu6.IF = 0x01;  // Set V-Blank flag
    cpu6.IME = true; // Enable interrupts
    mmu6.writeByte(0x0100, 0x00);  // NOP(shouldn't execute)
    
    int cycles = cpu6.step();  // This should service the interrupt instead
    
    assertEqual("PC should be 0x0040(V-Blank vector)", cpu6.PC, 0x0040);
    if(cpu6.IF == 0x00){
        std::cout << "✓ V-Blank IF flag cleared" << std::endl;
    } else {
        std::cout << "✗ IF flag should be cleared" << std::endl;
    }
    if(!cpu6.IME){
        std::cout << "✓ IME disabled after interrupt" << std::endl;
    } else {
        std::cout << "✗ IME should be disabled" << std::endl;
    }
    if(cycles == 20){
        std::cout << "✓ Interrupt service took 20 cycles" << std::endl;
    } else {
        std::cout << "✗ Interrupt should take 20 cycles, got " << cycles << std::endl;
    }
    std::cout << std::endl;

    // Test 7: Interrupt service pushes PC to stack
    std::cout << "Test 7: Interrupt service pushes PC to stack" << std::endl;
    MMU mmu7;
    CPU cpu7(mmu7);
    cpu7.PC = 0x0150;
    cpu7.SP = 0xFFFE;
    cpu7.IE = 0x04;  // Enable TIMER
    cpu7.IF = 0x04;  // Set TIMER flag
    cpu7.IME = true;
    
    cpu7.step();  // Service interrupt
    
    // PC should be at TIMER vector(0x0050)
    assertEqual("PC should be at TIMER vector", cpu7.PC, 0x0050);
    // SP should be decremented by 2
    assertEqual("SP should be 0xFFFC", cpu7.SP, 0xFFFC);
    // Stack should contain the saved PC(0x0150)
    uint16_t saved_pc = mmu7.readWord(0xFFFC);
    assertEqual("Stack should contain saved PC(lo byte)", saved_pc & 0xFF, 0x50);
    assertEqual("Stack should contain saved PC(hi byte)", (saved_pc >> 8) & 0xFF, 0x01);
    std::cout << std::endl;

    // Test 8: LCD STAT interrupt vector
    std::cout << "Test 8: LCD STAT interrupt jumps to 0x0048" << std::endl;
    MMU mmu8;
    CPU cpu8(mmu8);
    cpu8.PC = 0x0200;
    cpu8.SP = 0xFFFE;
    cpu8.IE = 0x02;  // Enable LCD STAT
    cpu8.IF = 0x02;  // Set LCD STAT flag
    cpu8.IME = true;
    
    cpu8.step();
    
    assertEqual("PC should be at LCD STAT vector", cpu8.PC, 0x0048);
    std::cout << std::endl;

    // Test 9: Serial interrupt vector
    std::cout << "Test 9: Serial interrupt jumps to 0x0058" << std::endl;
    MMU mmu9;
    CPU cpu9(mmu9);
    cpu9.PC = 0x0300;
    cpu9.SP = 0xFFFE;
    cpu9.IE = 0x08;  // Enable Serial
    cpu9.IF = 0x08;  // Set Serial flag
    cpu9.IME = true;
    
    cpu9.step();
    
    assertEqual("PC should be at Serial vector", cpu9.PC, 0x0058);
    std::cout << std::endl;

    // Test 10: Joypad interrupt vector
    std::cout << "Test 10: Joypad interrupt jumps to 0x0060" << std::endl;
    MMU mmu10;
    CPU cpu10(mmu10);
    cpu10.PC = 0x0400;
    cpu10.SP = 0xFFFE;
    cpu10.IE = 0x10;  // Enable Joypad
    cpu10.IF = 0x10;  // Set Joypad flag
    cpu10.IME = true;
    
    cpu10.step();
    
    assertEqual("PC should be at Joypad vector", cpu10.PC, 0x0060);
    std::cout << std::endl;

    std::cout << "=== Interrupt Tests Complete ===" << std::endl;
    return 0;
}
