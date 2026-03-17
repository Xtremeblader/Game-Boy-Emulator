#include "MMU.h"
#include "CPU.h"
#include <iostream>
#include <iomanip>

void assertEqual(const std::string& test, uint16_t got, uint16_t expected){
    if(got == expected){
        std::cout << "✓ " << test << std::endl;
    } else {
        std::cout << "✗ " << test << " - got 0x" << std::hex << got 
                  << " expected 0x" << expected << std::dec << std::endl;
    }
}

void assertFlagSet(const std::string& test, CPU& cpu, int flagBit){
    CPU::FlagBit flag = static_cast<CPU::FlagBit>(flagBit);
    if(cpu.getFlag(flag)){
        std::cout << "✓ " << test << std::endl;
    } else {
        std::cout << "✗ " << test << " - flag not set" << std::endl;
    }
}

void assertFlagClear(const std::string& test, CPU& cpu, int flagBit){
    CPU::FlagBit flag = static_cast<CPU::FlagBit>(flagBit);
    if(!cpu.getFlag(flag)){
        std::cout << "✓ " << test << std::endl;
    } else {
        std::cout << "✗ " << test << " - flag not cleared" << std::endl;
    }
}

int main(){
    std::cout << "=== Game Boy CPU Opcode Tests ===" << std::endl << std::endl;

    MMU mmu;
    CPU cpu(mmu);

    // Test 1: NOP(0x00)
    std::cout << "Test 1: NOP" << std::endl;
    cpu.step();  // Should just return 4 cycles
    std::cout << "✓ NOP executed" << std::endl << std::endl;

    // Test 2: LD BC,d16(0x01)
    std::cout << "Test 2: LD BC,d16 with value 0x1234" << std::endl;
    mmu.writeByte(0x0100, 0x01);      // LD BC,d16
    mmu.writeByte(0x0101, 0x34);      // low byte
    mmu.writeByte(0x0102, 0x12);      // high byte
    CPU cpu2(mmu);
    cpu2.step();
    assertEqual("BC should be 0x1234", cpu2.BC.reg, 0x1234);
    std::cout << std::endl;

    // Test 3: LD A,d8(0x3E)
    std::cout << "Test 3: LD A,d8 with value 0xAB" << std::endl;
    mmu.writeByte(0x0100, 0x3E);      // LD A,d8
    mmu.writeByte(0x0101, 0xAB);      // value
    CPU cpu3(mmu);
    cpu3.step();
    assertEqual("A should be 0xAB", cpu3.AF.bytes.hi, 0xAB);
    std::cout << std::endl;

    // Test 4: ADD A,d8 and flags(0xC6)
    std::cout << "Test 4: ADD A,d8(0x50 + 0x30 = 0x80)" << std::endl;
    mmu.writeByte(0x0100, 0x3E);      // LD A,d8
    mmu.writeByte(0x0101, 0x50);
    mmu.writeByte(0x0102, 0xC6);      // ADD A,d8
    mmu.writeByte(0x0103, 0x30);
    CPU cpu4(mmu);
    cpu4.step();  // LD A,d8
    cpu4.step();  // ADD A,d8
    assertEqual("A should be 0x80", cpu4.AF.bytes.hi, 0x80);
    assertFlagClear("ZERO flag should be clear", cpu4, CPU::ZERO);
    assertFlagClear("CARRY flag should be clear", cpu4, CPU::CARRY);
    std::cout << std::endl;

    // Test 5: ADD A with carry(0xC6)
    std::cout << "Test 5: ADD A,d8(0xFF + 0x01 = 0x00, with carry)" << std::endl;
    mmu.writeByte(0x0100, 0x3E);      // LD A,d8
    mmu.writeByte(0x0101, 0xFF);
    mmu.writeByte(0x0102, 0xC6);      // ADD A,d8
    mmu.writeByte(0x0103, 0x01);
    CPU cpu5(mmu);
    cpu5.step();  // LD A,0xFF
    cpu5.step();  // ADD A,d8(0xFF + 0x01)
    assertEqual("A should be 0x00", cpu5.AF.bytes.hi, 0x00);
    assertFlagSet("ZERO flag should be set", cpu5, CPU::ZERO);
    assertFlagSet("CARRY flag should be set", cpu5, CPU::CARRY);
    std::cout << std::endl;

    // Test 6: XOR A,A(0xAF)
    std::cout << "Test 6: XOR A,A(should clear A and set ZERO)" << std::endl;
    mmu.writeByte(0x0100, 0x3E);      // LD A,d8
    mmu.writeByte(0x0101, 0xFF);
    mmu.writeByte(0x0102, 0xAF);      // XOR A,A
    CPU cpu6(mmu);
    cpu6.step();  // LD A,0xFF
    cpu6.step();  // XOR A,A
    assertEqual("A should be 0x00", cpu6.AF.bytes.hi, 0x00);
    assertFlagSet("ZERO flag should be set", cpu6, CPU::ZERO);
    std::cout << std::endl;

    // Test 7: INC A(0x3C)
    std::cout << "Test 7: INC A(0x00 -> 0x01)" << std::endl;
    mmu.writeByte(0x0100, 0x3E);      // LD A,d8
    mmu.writeByte(0x0101, 0x00);
    mmu.writeByte(0x0102, 0x3C);      // INC A
    CPU cpu7(mmu);
    cpu7.step();  // LD A,0x00
    cpu7.step();  // INC A
    assertEqual("A should be 0x01", cpu7.AF.bytes.hi, 0x01);
    assertFlagClear("ZERO flag should be clear", cpu7, CPU::ZERO);
    std::cout << std::endl;

    // Test 8: DEC A(0x3D)
    std::cout << "Test 8: DEC A(0x01 -> 0x00)" << std::endl;
    mmu.writeByte(0x0100, 0x3E);      // LD A,d8
    mmu.writeByte(0x0101, 0x01);
    mmu.writeByte(0x0102, 0x3D);      // DEC A
    CPU cpu8(mmu);
    cpu8.step();  // LD A,0x01
    cpu8.step();  // DEC A
    assertEqual("A should be 0x00", cpu8.AF.bytes.hi, 0x00);
    assertFlagSet("ZERO flag should be set", cpu8, CPU::ZERO);
    std::cout << std::endl;

    // Test 9: JP a16(0xC3)
    std::cout << "Test 9: JP a16(jump to 0x0150)" << std::endl;
    mmu.writeByte(0x0100, 0xC3);      // JP a16
    mmu.writeByte(0x0101, 0x50);      // low byte
    mmu.writeByte(0x0102, 0x01);      // high byte
    CPU cpu9(mmu);
    cpu9.step();  // JP a16
    assertEqual("PC should be 0x0150", cpu9.PC, 0x0150);
    std::cout << std::endl;

    // Test 10: PUSH/POP BC
    std::cout << "Test 10: PUSH BC / POP BC" << std::endl;
    mmu.writeByte(0x0100, 0x01);      // LD BC,d16
    mmu.writeByte(0x0101, 0x34);
    mmu.writeByte(0x0102, 0x12);
    mmu.writeByte(0x0103, 0xC5);      // PUSH BC
    mmu.writeByte(0x0104, 0x01);      // LD BC,0000
    mmu.writeByte(0x0105, 0x00);
    mmu.writeByte(0x0106, 0x00);
    mmu.writeByte(0x0107, 0xC1);      // POP BC
    CPU cpu10(mmu);
    cpu10.step();  // LD BC,0x1234
    cpu10.step();  // PUSH BC
    cpu10.step();  // LD BC,0000
    assertEqual("BC should be 0x0000 after LD", cpu10.BC.reg, 0x0000);
    cpu10.step();  // POP BC
    assertEqual("BC should be 0x1234 after POP", cpu10.BC.reg, 0x1234);
    std::cout << std::endl;

    std::cout << "=== Tests Complete ===" << std::endl;
    return 0;
}
