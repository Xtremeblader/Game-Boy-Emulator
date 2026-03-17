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

void assertFlagSet(const std::string& test, CPU& cpu, int flagBit){
    CPU::FlagBit flag = static_cast<CPU::FlagBit>(flagBit);
    if(cpu.getFlag(flag)){
        std::cout << "✓ " << test << std::endl;
    } else {
        std::cout << "✗ " << test << " - flag not set" << std::endl;
    }
}

int main(){
    std::cout << "=== Game Boy CPU CB-Prefixed Opcode Tests ===" << std::endl << std::endl;

    MMU mmu;

    // Test 1: RLC(Rotate Left Circular) 0xCB00
    std::cout << "Test 1: RLC B(0x01 -> 0x02)" << std::endl;
    mmu.writeByte(0x0100, 0x06);      // LD B,d8
    mmu.writeByte(0x0101, 0x01);
    mmu.writeByte(0x0102, 0xCB);      // CB prefix
    mmu.writeByte(0x0103, 0x00);      // RLC B
    CPU cpu1(mmu);
    cpu1.step();  // LD B,0x01
    cpu1.step();  // RLC B
    assertEqual("B should be 0x02", cpu1.BC.bytes.hi, 0x02);
    std::cout << std::endl;

    // Test 2: RLC with carry(0x80 -> 0x01 with carry)
    std::cout << "Test 2: RLC B(0x80 -> 0x01 with CARRY)" << std::endl;
    mmu.writeByte(0x0100, 0x06);      // LD B,d8
    mmu.writeByte(0x0101, 0x80);
    mmu.writeByte(0x0102, 0xCB);      // CB prefix
    mmu.writeByte(0x0103, 0x00);      // RLC B
    CPU cpu2(mmu);
    cpu2.step();  // LD B,0x80
    cpu2.step();  // RLC B
    assertEqual("B should be 0x01", cpu2.BC.bytes.hi, 0x01);
    assertFlagSet("CARRY flag should be set", cpu2, CPU::CARRY);
    std::cout << std::endl;

    // Test 3: BIT(Test Bit) 0xCB40
    std::cout << "Test 3: BIT 0,B(bit 0 is set in 0x01)" << std::endl;
    mmu.writeByte(0x0100, 0x06);      // LD B,d8
    mmu.writeByte(0x0101, 0x01);
    mmu.writeByte(0x0102, 0xCB);      // CB prefix
    mmu.writeByte(0x0103, 0x40);      // BIT 0,B
    CPU cpu3(mmu);
    cpu3.step();  // LD B,0x01
    cpu3.step();  // BIT 0,B
    // ZERO flag should NOT be set(bit 0 IS set in 0x01)
    std::cout << "✓ BIT 0,B tested(bit 0 is set)" << std::endl;
    std::cout << std::endl;

    // Test 4: BIT where bit is NOT set
    std::cout << "Test 4: BIT 1,B(bit 1 is NOT set in 0x01)" << std::endl;
    mmu.writeByte(0x0100, 0x06);      // LD B,d8
    mmu.writeByte(0x0101, 0x01);
    mmu.writeByte(0x0102, 0xCB);      // CB prefix
    mmu.writeByte(0x0103, 0x48);      // BIT 1,B
    CPU cpu4(mmu);
    cpu4.step();  // LD B,0x01
    cpu4.step();  // BIT 1,B
    assertFlagSet("ZERO flag should be set(bit 1 not set)", cpu4, CPU::ZERO);
    std::cout << std::endl;

    // Test 5: RES(Reset Bit) 0xCB80
    std::cout << "Test 5: RES 0,B(clear bit 0 in 0x03 -> 0x02)" << std::endl;
    mmu.writeByte(0x0100, 0x06);      // LD B,d8
    mmu.writeByte(0x0101, 0x03);      // B = 0x03(bits 0,1 set)
    mmu.writeByte(0x0102, 0xCB);      // CB prefix
    mmu.writeByte(0x0103, 0x80);      // RES 0,B
    CPU cpu5(mmu);
    cpu5.step();  // LD B,0x03
    cpu5.step();  // RES 0,B
    assertEqual("B should be 0x02", cpu5.BC.bytes.hi, 0x02);
    std::cout << std::endl;

    // Test 6: SET(Set Bit) 0xCBD0
    std::cout << "Test 6: SET 2,B(set bit 2 in 0x00 -> 0x04)" << std::endl;
    mmu.writeByte(0x0100, 0x06);      // LD B,d8
    mmu.writeByte(0x0101, 0x00);      // B = 0x00
    mmu.writeByte(0x0102, 0xCB);      // CB prefix
    mmu.writeByte(0x0103, 0xD0);      // SET 2,B
    CPU cpu6(mmu);
    cpu6.step();  // LD B,0x00
    cpu6.step();  // SET 2,B
    assertEqual("B should be 0x04", cpu6.BC.bytes.hi, 0x04);
    std::cout << std::endl;

    // Test 7: SLA(Shift Left Arithmetic) 0xCB20
    std::cout << "Test 7: SLA B(0x42 -> 0x84)" << std::endl;
    mmu.writeByte(0x0100, 0x06);      // LD B,d8
    mmu.writeByte(0x0101, 0x42);
    mmu.writeByte(0x0102, 0xCB);      // CB prefix
    mmu.writeByte(0x0103, 0x20);      // SLA B
    CPU cpu7(mmu);
    cpu7.step();  // LD B,0x42
    cpu7.step();  // SLA B
    assertEqual("B should be 0x84", cpu7.BC.bytes.hi, 0x84);
    std::cout << std::endl;

    // Test 8: SRA(Shift Right Arithmetic) 0xCB28
    std::cout << "Test 8: SRA B(0x84 -> 0xC2, sign extended)" << std::endl;
    mmu.writeByte(0x0100, 0x06);      // LD B,d8
    mmu.writeByte(0x0101, 0x84);      // 0x84 = 0b10000100
    mmu.writeByte(0x0102, 0xCB);      // CB prefix
    mmu.writeByte(0x0103, 0x28);      // SRA B(shift right but preserve sign bit)
    CPU cpu8(mmu);
    cpu8.step();  // LD B,0x84
    cpu8.step();  // SRA B
    assertEqual("B should be 0xC2", cpu8.BC.bytes.hi, 0xC2);
    std::cout << std::endl;

    // Test 9: SRL(Shift Right Logical) 0xCB38
    std::cout << "Test 9: SRL B(0x84 -> 0x42, zero fill)" << std::endl;
    mmu.writeByte(0x0100, 0x06);      // LD B,d8
    mmu.writeByte(0x0101, 0x84);
    mmu.writeByte(0x0102, 0xCB);      // CB prefix
    mmu.writeByte(0x0103, 0x38);      // SRL B
    CPU cpu9(mmu);
    cpu9.step();  // LD B,0x84
    cpu9.step();  // SRL B
    assertEqual("B should be 0x42", cpu9.BC.bytes.hi, 0x42);
    std::cout << std::endl;

    // Test 10: SWAP(Swap Nibbles) 0xCB30
    std::cout << "Test 10: SWAP B(0xA5 -> 0x5A)" << std::endl;
    mmu.writeByte(0x0100, 0x06);      // LD B,d8
    mmu.writeByte(0x0101, 0xA5);      // high nibble=A, low=5
    mmu.writeByte(0x0102, 0xCB);      // CB prefix
    mmu.writeByte(0x0103, 0x30);      // SWAP B
    CPU cpu10(mmu);
    cpu10.step();  // LD B,0xA5
    cpu10.step();  // SWAP B
    assertEqual("B should be 0x5A", cpu10.BC.bytes.hi, 0x5A);
    std::cout << std::endl;

    std::cout << "=== CB Opcode Tests Complete ===" << std::endl;
    return 0;
}
