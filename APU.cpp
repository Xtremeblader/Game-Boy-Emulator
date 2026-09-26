#include "APU.h"
#include <algorithm>
#include <utility>

uint8_t APU::read(uint16_t address) const {
    if(address < 0xFF10 || address > 0xFF3F) return 0xFF;
    int r = address - 0xFF10;
    if(r >= 0x20) return regs[r];
    if(r == 0x16){
        uint8_t status = powered ? 0xF0 : 0x70;
        for(int i=0;i<4;++i) if(channels[i].enabled) status |= 1 << i;
        return status;
    }
    constexpr uint8_t masks[] = {
        0x80,0x3F,0x00,0xFF,0xBF,0xFF,0x3F,0x00,0xFF,0xBF,
        0x7F,0xFF,0x9F,0xFF,0xBF,0xFF,0xFF,0x00,0x00,0xBF,
        0x00,0x00,0x70,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
    return regs[r] | masks[r];
}

int APU::period(int i) const {
    if(i < 2) return (2048 - channels[i].frequency) * 4;
    if(i == 2) return (2048 - channels[i].frequency) * 2;
    int divisor = regs[0x12] & 7;
    return (divisor ? divisor * 16 : 8) << (regs[0x12] >> 4);
}

int APU::sweepValue(){
    int delta = sweep_shadow >> (regs[0] & 7);
    if(regs[0] & 8){ sweep_negated = true; return sweep_shadow - delta; }
    return sweep_shadow + delta;
}

void APU::trigger(int i){
    auto& c = channels[i];
    c.enabled = c.dac;
    if(!c.length){
        c.length = i == 2 ? 256 : 64;
        if(c.length_enabled && (frame_step & 1)) --c.length;
    }
    c.timer = period(i);
    if(i == 2) c.position = 0;
    if(i == 3) lfsr = 0x7FFF;
    if(i != 2){
        int envelope = regs[i == 3 ? 0x11 : i*5+2];
        c.volume = envelope >> 4;
        c.envelope_timer = (envelope & 7) ? envelope & 7 : 8;
    }
    if(i == 0){
        sweep_shadow = c.frequency;
        sweep_timer = (regs[0] >> 4) & 7;
        if(!sweep_timer) sweep_timer = 8;
        sweep_enabled = (regs[0] & 0x77) != 0;
        sweep_negated = false;
        if((regs[0] & 7) && sweepValue() > 2047) c.enabled = false;
    }
}

void APU::write(uint16_t address, uint8_t value){
    if(address < 0xFF10 || address > 0xFF3F) return;
    int r = address - 0xFF10;
    if(r >= 0x20){ regs[r] = value; return; }
    if(r == 0x16){
        bool next = value & 0x80;
        if(powered && !next){
            std::fill(regs.begin(),regs.begin()+0x16,0);
            for(auto& c:channels){ int length=c.length; c=Channel{}; c.length=length; }
            sweep_enabled = sweep_negated = false;
        }
        if(!powered && next) frame_step = 0;
        powered = next;
        return;
    }
    if(r >= 0x17 || r == 5 || r == 0x0F) return;
    int i = r < 5 ? 0 : r < 10 ? 1 : r < 15 ? 2 : 3;
    int base = i*5;
    if(!powered){
        // DMG length counters remain writable while powered down.
        if(r == base+1) channels[i].length = i==2 ? 256-value : 64-(value&63);
        return;
    }
    uint8_t old = regs[r];
    regs[r] = value;
    if(r >= 0x14) return;
    auto& c = channels[i];
    if(r == 0 && (old & 8) && !(value & 8) && sweep_negated) c.enabled=false;
    if(r == base+1) c.length = i==2 ? 256-value : 64-(value&63);
    if((i==2 && r==0x0A) || (i!=2 && r==base+2)){
        c.dac = i==2 ? (value&0x80)!=0 : (value&0xF8)!=0;
        if(!c.dac) c.enabled=false;
    }
    if(i < 3 && (r==base+3 || r==base+4))
        c.frequency = regs[base+3] | ((regs[base+4]&7)<<8);
    if(r == base+4){
        bool enable = value & 0x40;
        if(!c.length_enabled && enable && (frame_step&1) && c.length && --c.length==0)
            c.enabled=false;
        c.length_enabled=enable;
        if(value & 0x80) trigger(i);
    }
}

void APU::sweep(){
    if(--sweep_timer > 0) return;
    sweep_timer = (regs[0]>>4)&7;
    if(!sweep_timer) sweep_timer=8;
    if(!sweep_enabled || !(regs[0]&0x70)) return;
    int next=sweepValue();
    if(next>2047){ channels[0].enabled=false; return; }
    if(regs[0]&7){
        sweep_shadow=channels[0].frequency=next;
        regs[3]=next&255; regs[4]=(regs[4]&0xF8)|(next>>8);
        if(sweepValue()>2047) channels[0].enabled=false;
    }
}

void APU::frameTick(){
    if(!powered) return;
    if(!(frame_step&1)) for(auto& c:channels)
        if(c.length_enabled && c.length && --c.length==0) c.enabled=false;
    if(frame_step==2 || frame_step==6) sweep();
    if(frame_step==7){
        for(int i : {0,1,3}){
            auto& c=channels[i];
            int env=regs[i*5+2];
            if(c.enabled && (env&7) && --c.envelope_timer<=0){
                c.envelope_timer=env&7;
                c.volume=std::clamp(c.volume+((env&8)?1:-1),0,15);
            }
        }
    }
    frame_step=(frame_step+1)&7;
}

void APU::resetDivider(){
    if(divider&0x1000) frameTick();
    divider=0;
}

float APU::output(int i) const {
    const auto& c=channels[i];
    if(!powered || !c.dac || !c.enabled) return 0;
    int value=0;
    if(i<2){
        constexpr uint8_t duties[]={0x01,0x81,0x87,0x7E};
        value=((duties[regs[i*5+1]>>6]>>c.position)&1)?c.volume:0;
    } else if(i==2){
        int shift=(regs[0x0C]>>5)&3;
        if(!shift) return 0;
        int byte=regs[0x20+c.position/2];
        value=((c.position&1)?byte&15:byte>>4)>>(shift-1);
    } else value=(lfsr&1)?0:c.volume;
    return static_cast<float>(value)/7.5f-1.0f;
}

void APU::sample(){
    float left=0,right=0;
    for(int i=0;i<4;++i){
        float value=output(i);
        if(regs[0x15]&(1<<(i+4))) left+=value;
        if(regs[0x15]&(1<<i)) right+=value;
    }
    sum_left+=left*((regs[0x14]>>4&7)+1)/32.0;
    sum_right+=right*((regs[0x14]&7)+1)/32.0;
    ++sample_clocks;
    sample_phase+=SampleRate;
    if(sample_phase>=4194304){
        sample_phase-=4194304;
        float l=static_cast<float>(sum_left/sample_clocks);
        float r=static_cast<float>(sum_right/sample_clocks);
        // Simple DC-removal filter; conservative output gain avoids loud startup.
        float filtered_l=l-capacitor_left, filtered_r=r-capacitor_right;
        capacitor_left=l-filtered_l*0.996f; capacitor_right=r-filtered_r*0.996f;
        if(samples.size()<SampleRate*2){
            samples.push_back(filtered_l*0.35f); samples.push_back(filtered_r*0.35f);
        }
        sum_left=sum_right=0; sample_clocks=0;
    }
}

void APU::update(int cycles){
    for(int tick=0;tick<cycles;++tick){
        bool high=divider&0x1000;
        ++divider;
        if(high && !(divider&0x1000)) frameTick();
        if(powered) for(int i=0;i<4;++i){
            auto& c=channels[i];
            if(!c.enabled) continue;
            if(--c.timer<=0){
                c.timer=period(i);
                if(i<2) c.position=(c.position+1)&7;
                else if(i==2) c.position=(c.position+1)&31;
                else {
                    int bit=(lfsr^(lfsr>>1))&1;
                    lfsr=(lfsr>>1)|(bit<<14);
                    if(regs[0x12]&8) lfsr=(lfsr&~0x40)|(bit<<6);
                }
            }
        }
        sample();
    }
}

std::vector<float> APU::takeSamples(){
    std::vector<float> result;
    result.swap(samples);
    return result;
}
