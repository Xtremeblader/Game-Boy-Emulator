#include "MMU.h"
#include "CPU.h"
#include "PPU.h"
#include "Display.h"
#include "Joypad.h"
#include "Timer.h"
#include "APU.h"
#include "AudioOutput.h"
#include "Cartridge.h"
#include <csignal>
#include <chrono>
#include <fstream>
#include <iostream>
#include <string>
#include <thread>

namespace {
volatile std::sig_atomic_t stop_requested = 0;
void requestStop(int){ stop_requested = 1; }
}

int main(int argc, char* argv[]){
    bool headless = false, pattern = false, mute = false;
    unsigned long long limit = 0;
    std::string rom = "Tetris (JUE) (V1.1) [!].gb", screenshot;
    try {
        for(int i = 1; i < argc; ++i){
            std::string arg = argv[i];
            if(arg == "--headless") headless = true;
            else if(arg == "--mute") mute = true;
            else if(arg == "--test-pattern") pattern = true;
            else if(arg == "--frames" && i + 1 < argc){
                std::string number = argv[++i];
                if(number.empty() || number.find_first_not_of("0123456789") != std::string::npos)
                    throw std::invalid_argument("frames");
                limit = std::stoull(number);
                if(!limit) throw std::invalid_argument("frames");
            } else if(arg == "--screenshot" && i + 1 < argc) screenshot = argv[++i];
            else if(arg == "--help"){
                std::cout << "Usage: emulator [ROM] [--headless] [--mute] [--frames N] [--test-pattern] [--screenshot image.ppm]\n";
                return 0;
            } else if(arg.rfind("--", 0) == 0) throw std::invalid_argument(arg);
            else rom = arg;
        }
    } catch(const std::exception&){
        std::cerr << "Invalid arguments. Use --help; --frames requires a positive integer.\n";
        return 1;
    }
    if(headless && !limit) limit = 60;
    MMU mmu;
    CPU cpu(mmu);
    PPU ppu(mmu);
    Joypad joypad;
    Timer timer;
    APU apu;
    mmu.linkAPU(&apu);
    mmu.linkTimer(&timer);
    mmu.linkJoypad(&joypad);
    mmu.linkPPU(&ppu);
    if(pattern){
        mmu.writeByte(0xFF47, 0xE4);
        // Four vertical shades in each tile, repeated across the background.
        for(int row = 0; row < 8; ++row){
            mmu.writeByte(0x8000 + row * 2, 0x33);
            mmu.writeByte(0x8001 + row * 2, 0x0F);
        }
    } else if(!mmu.loadCartridge(rom)) return 1;
    std::signal(SIGINT, requestStop);
    std::signal(SIGTERM, requestStop);
    Display display;
    if(!headless && !display.open()){
        std::cerr << "Display error: " << display.error() << '\n';
        return 1;
    }
    AudioOutput audio;
    bool audio_enabled = !headless && !mute;
    if(audio_enabled && !audio.open()){
        std::cerr << "Audio unavailable: " << audio.error() << ". Continuing muted.\n";
        audio_enabled = false;
    }
    using Clock = std::chrono::steady_clock;
    auto deadline = Clock::now();
    const auto frame_time = std::chrono::duration_cast<Clock::duration>(
        std::chrono::duration<double>(70224.0 / 4194304.0));
    unsigned long long frames = 0, cycles = 0;
    int budget = 0;
    // Host frames keep events responsive even when the ROM disables the LCD.
    while(!stop_requested && (!limit || frames < limit)){
        if(!headless && !display.poll(joypad)) break;
        mmu.syncJoypadInterrupt();
        budget += 70224;
        while(budget > 0){
            int elapsed = pattern ? 4 : cpu.step();
            if(elapsed <= 0){ std::cerr << "CPU returned invalid timing\n"; return 1; }
            mmu.updateTimer(elapsed);
            apu.update(elapsed);
            mmu.updateDMA(elapsed);
            ppu.update(elapsed);
            cycles += elapsed;
            budget -= elapsed;
            if(ppu.getVBlankInterrupt()){
                cpu.IF |= 1 << CPU::V_BLANK;
                ppu.clearVBlankInterrupt();
            }
            if(ppu.getLcdStatInterrupt()){
                cpu.IF |= 1 << CPU::LCD_STAT;
                ppu.clearLcdStatInterrupt();
            }
            if(ppu.hasNewFrame()){
                if(!headless && !display.present(ppu.getFramebuffer())){
                    std::cerr << "Display error: " << display.error() << '\n';
                    return 1;
                }
                ppu.resetFrameReady();
            }
        }
        auto samples = apu.takeSamples();
        if(audio_enabled && !audio.queue(samples)){
            std::cerr << "Audio output failed: " << audio.error() << ". Continuing muted.\n";
            audio_enabled = false;
        }
        ++frames;
        if(frames % 300 == 0 && mmu.getCartridge() && !mmu.getCartridge()->saveBattery()) return 1;
        if(!headless){
            deadline += frame_time;
            std::this_thread::sleep_until(deadline);
            if(Clock::now() - deadline > frame_time * 4) deadline = Clock::now();
        }
    }
    if(mmu.getCartridge() && !mmu.getCartridge()->saveBattery()) return 1;
    if(!screenshot.empty()){
        std::ofstream output(screenshot, std::ios::binary);
        output << "P6\n160 144\n255\n";
        for(uint16_t color : ppu.getFramebuffer()){
            for(int shift : {10, 5, 0}){
                unsigned c = (color >> shift) & 31;
                output.put(static_cast<char>((c << 3) | (c >> 2)));
            }
        }
        if(!output){ std::cerr << "Failed to write screenshot\n"; return 1; }
    }
    std::cout << "Emulation stopped: " << frames << " host frames, " << cycles << " CPU clock cycles.\n";
}
