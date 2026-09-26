#pragma once
#include <array>
#include <cstdint>
#include <vector>

// DMG audio, clocked in CPU T-cycles. Output is interleaved stereo at 48 kHz.
class APU {
public:
    static constexpr int SampleRate = 48000;
    uint8_t read(uint16_t address) const;
    void write(uint16_t address, uint8_t value);
    void update(int cycles);
    void resetDivider();
    std::vector<float> takeSamples();
private:
    struct Channel {
        bool enabled = false, dac = false, length_enabled = false;
        int length = 0, frequency = 0, timer = 1, position = 0;
        int volume = 0, envelope_timer = 0;
    };
    std::array<Channel, 4> channels{};
    std::array<uint8_t, 0x30> regs = []{
        std::array<uint8_t, 0x30> r{};
        r[0x14] = 0x77; r[0x15] = 0xF3;
        return r;
    }();
    bool powered = true;
    uint16_t divider = 0, lfsr = 0x7FFF;
    int frame_step = 0, sample_phase = 0, sample_clocks = 0;
    int sweep_shadow = 0, sweep_timer = 0;
    bool sweep_enabled = false, sweep_negated = false;
    double sum_left = 0, sum_right = 0;
    float capacitor_left = 0, capacitor_right = 0;
    std::vector<float> samples;
    int period(int channel) const;
    void trigger(int channel);
    void frameTick();
    void sweep();
    int sweepValue();
    float output(int channel) const;
    void sample();
};
