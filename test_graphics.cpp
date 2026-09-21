#include "CPU.h"
#include "PPU.h"
#include <cassert>
#include <iostream>

int main(){
    MMU mmu;
    CPU cpu(mmu);
    PPU ppu(mmu);
    mmu.linkPPU(&ppu);
    mmu.writeByte(0xFFFF, 3);
    assert(cpu.IE == 3 && mmu.readByte(0xFFFF) == 3);
    mmu.writeByte(0xFF47, 0xE4);
    assert(ppu.readBGP() == 0xE4);
    mmu.writeByte(0x8000, 0x33);
    mmu.writeByte(0x8001, 0x0F);
    ppu.update(80);
    assert(ppu.getMode() == PPU::PIXEL_TRANSFER);
    ppu.update(172);
    const auto& fb = ppu.getFramebuffer();
    assert(fb[0] == 0x7FFF && fb[2] == 0x56B5 && fb[4] == 0x294A && fb[6] == 0);
    ppu.update(203);
    assert(ppu.readLY() == 0);
    ppu.update(1);
    assert(mmu.readByte(0xFF44) == 1);
    mmu.writeByte(0xFF44, 0);
    assert(ppu.readLY() == 1);
    ppu.update(456 * 143);
    assert(ppu.readLY() == 144 && ppu.hasNewFrame() && ppu.getVBlankInterrupt());
    ppu.clearVBlankInterrupt();
    ppu.resetFrameReady();
    ppu.update(456 * 10);
    assert(ppu.readLY() == 0 && ppu.getMode() == PPU::OAM_SEARCH);
    assert(!ppu.getVBlankInterrupt() && !ppu.hasNewFrame());
    // STAT is a rising-edge signal, including register writes.
    mmu.writeByte(0xFF41, 0x20);
    assert(ppu.getLcdStatInterrupt());
    ppu.clearLcdStatInterrupt();
    mmu.writeByte(0xFF41, 0x20);
    assert(!ppu.getLcdStatInterrupt());
    ppu.update(456);
    assert(ppu.getLcdStatInterrupt());
    mmu.writeByte(0xFF40, 0);
    ppu.update(70224);
    assert(ppu.readLY() == 0 && fb[0] == 0x7FFF);
    // Signed tile addressing: index FF selects tile at 8FF0.
    mmu.writeByte(0x9800, 0xFF);
    mmu.writeByte(0x8FF0, 0xFF);
    mmu.writeByte(0x8FF1, 0xFF);
    mmu.writeByte(0xFF40, 0x81);
    ppu.update(252);
    assert(fb[0] == 0);
    // Background disable produces white regardless of palette.
    mmu.writeByte(0xFF40, 0);
    mmu.writeByte(0xFF40, 0x90);
    ppu.update(252);
    assert(fb[0] == 0x7FFF);
    std::cout << "Graphics regression tests passed\n";
}
