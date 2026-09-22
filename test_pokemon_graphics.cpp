#include "CPU.h"
#include "PPU.h"
#include <cassert>
#include <iostream>

struct Scene {
    MMU mmu;
    PPU ppu{mmu};
    Scene(){
        mmu.linkPPU(&ppu);
        ppu.writeLCDC(0);
        ppu.writeBGP(0xE4);
        ppu.writeOBP0(0xE4);
        ppu.writeOBP1(0xFC);
    }
    void tile(int id, uint8_t low, uint8_t high){
        for(int y = 0; y < 8; ++y){
            mmu.writeByte(0x8000 + id * 16 + y * 2, low);
            mmu.writeByte(0x8001 + id * 16 + y * 2, high);
        }
    }
    void sprite(int i, int x, int y, int tile, int flags = 0){
        int base = 0xFE00 + i * 4;
        mmu.writeByte(base, y + 16);
        mmu.writeByte(base + 1, x + 8);
        mmu.writeByte(base + 2, tile);
        mmu.writeByte(base + 3, flags);
    }
    void draw(uint8_t lcdc = 0x93){ ppu.writeLCDC(lcdc); ppu.update(252); }
    uint16_t pixel(int x, int y = 0){ return ppu.getFramebuffer()[y * 160 + x]; }
};

int main(){
    // CP must replace carry, preserve A, and set every subtraction flag correctly.
    MMU memory;
    CPU cpu(memory);
    memory.writeByte(0xC000, 0xFE);
    for(int a = 0; a < 256; ++a){
        for(int b = 0; b < 256; ++b){
            for(int old_carry : {0, 0x10}){
                cpu.PC = 0xC000;
                cpu.AF.reg = (a << 8) | old_carry;
                memory.writeByte(0xC001, b);
                assert(cpu.step() == 8);
                int flags = 0x40 | (a == b ? 0x80 : 0) |
                            ((a & 15) < (b & 15) ? 0x20 : 0) | (a < b ? 0x10 : 0);
                assert(cpu.AF.bytes.hi == a && cpu.AF.bytes.lo == flags);
            }
        }
    }
    // DMA copies one byte per four clocks, ends at 160, and can restart.
    for(int i = 0; i < 160; ++i) memory.writeByte(0xC100 + i, i + 1);
    memory.writeByte(0xFF46, 0xC1);
    memory.updateDMA(3);
    assert(memory.readByte(0xFE00) == 0);
    memory.updateDMA(1);
    assert(memory.readByte(0xFE00) == 1 && memory.readByte(0xFE01) == 0);
    memory.updateDMA(636);
    assert(memory.readByte(0xFE9F) == 160 && memory.readByte(0xFF46) == 0xC1);
    memory.writeByte(0xC200, 0xAA);
    memory.writeByte(0xFF46, 0xC2);
    memory.updateDMA(4);
    assert(memory.readByte(0xFE00) == 0xAA);
    memory.writeByte(0xFF46, 0xC1);
    memory.updateDMA(640);
    assert(memory.readByte(0xFE00) == 1 && memory.readByte(0xFE9F) == 160);

    {
        Scene s;
        s.tile(1, 0x80, 0); // Leftmost pixel opaque, others transparent.
        s.sprite(0, 0, 0, 1);
        s.draw();
        assert(s.pixel(0) == 0x56B5 && s.pixel(1) == 0x7FFF);
    }
    {
        Scene s;
        s.tile(1, 0x80, 0);
        s.sprite(0, 0, 0, 1, 0x30); // X flip, palette 1.
        s.draw(0x92); // Sprites still work with BG disabled.
        assert(s.pixel(0) == 0x7FFF && s.pixel(7) == 0);
    }
    {
        Scene s;
        s.tile(2, 0xFF, 0);
        s.tile(3, 0, 0xFF);
        s.sprite(0, 0, 0, 3, 0x40); // 8x16: ignore odd tile bit, flip whole sprite.
        s.draw(0x97);
        assert(s.pixel(0) == 0x294A);
        s.ppu.update(456 * 8);
        assert(s.pixel(0, 8) == 0x56B5);
    }
    {
        Scene s;
        s.tile(1, 0xFF, 0);
        s.tile(2, 0, 0xFF);
        s.sprite(0, 4, 0, 1);
        s.sprite(1, 0, 0, 2); // Smaller X wins despite later OAM index.
        s.draw();
        assert(s.pixel(4) == 0x294A);
    }
    {
        Scene s;
        s.tile(0, 0xFF, 0);
        s.tile(1, 0, 0xFF);
        s.sprite(0, 0, 0, 1, 0x80); // Behind nonzero BG, masks later OBJ.
        s.sprite(1, 0, 0, 1);
        s.draw();
        assert(s.pixel(0) == 0x56B5);
    }
    {
        Scene s;
        s.tile(1, 0xFF, 0);
        for(int i = 0; i < 10; ++i) s.sprite(i, -8, 0, 1);
        s.sprite(10, 0, 0, 1);
        s.draw();
        assert(s.pixel(0) == 0x7FFF); // Horizontally hidden entries consume slots.
    }
    {
        Scene s;
        s.tile(1, 0xFF, 0);
        s.mmu.writeByte(0x9C00, 1);
        s.ppu.writeWX(10); // Window begins at X = 3.
        s.ppu.writeWY(0);
        s.draw(0xF1);
        assert(s.pixel(2) == 0x7FFF && s.pixel(3) == 0x56B5);
        s.ppu.writeLCDC(0xD1); // Pause the window for one line.
        s.ppu.update(456);
        s.mmu.writeByte(0x8012, 0); // Row 1 is dark gray.
        s.mmu.writeByte(0x8013, 0xFF);
        s.ppu.writeLCDC(0xF1);
        s.ppu.update(456);
        assert(s.pixel(3, 2) == 0x294A);
    }
    std::cout << "Compare flags, DMA, sprites, and window tests passed\n";
}
