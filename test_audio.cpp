#include "APU.h"
#include "MMU.h"
#include <cassert>
#include <cmath>
#include <iostream>
#include <algorithm>
#ifdef GB_WITH_SDL
#include "AudioOutput.h"
#endif

void pulse(APU& a){
    a.write(0xFF26,0x80); a.write(0xFF24,0x77); a.write(0xFF25,0x11);
    a.write(0xFF11,0x80); a.write(0xFF12,0xF0);
    a.write(0xFF13,0xD6); a.write(0xFF14,0x86); // ~440 Hz
}
float peak(const std::vector<float>& s,int side=0){
    float result=0; for(size_t i=side;i<s.size();i+=2) result=std::max(result,std::abs(s[i]));
    return result;
}
int rises(const std::vector<float>& s){
    int count=0; for(size_t i=2;i<s.size();i+=2) if(s[i-2]<0 && s[i]>=0) ++count;
    return count;
}
int main(){
    APU a; MMU m; m.linkAPU(&a);
    m.writeByte(0xFF26,0); assert(m.readByte(0xFF26)==0x70);
    m.writeByte(0xFF12,0xF0); assert(m.readByte(0xFF12)==0);
    m.writeByte(0xFF30,0xAB); assert(m.readByte(0xFF30)==0xAB);
    pulse(a); assert((m.readByte(0xFF26)&0x81)==0x81);
    assert(m.readByte(0xFF13)==0xFF && m.readByte(0xFF27)==0xFF);
    a.update(4194304); auto tone=a.takeSamples();
    assert(tone.size()==96000 && rises(tone)>430 && rises(tone)<450);
    assert(peak(tone)>0.05f && peak(tone)<1.0f);
    for(size_t i=0;i<tone.size();++i) assert(std::isfinite(tone[i]));
    a.write(0xFF25,0x10); a.update(4194304/4); a.takeSamples();
    a.update(4194304/4); auto left=a.takeSamples();
    assert(peak(left,0)>0.05 && peak(left,1)<0.00001);
    a.write(0xFF12,0); assert(!(a.read(0xFF26)&1));
    a.write(0xFF14,0x80); assert(!(a.read(0xFF26)&1)); // DAC off blocks trigger.

    APU length; pulse(length);
    length.write(0xFF11,0xBF); length.write(0xFF14,0xC6);
    length.update(8191); assert(length.read(0xFF26)&1);
    length.update(1); assert(!(length.read(0xFF26)&1));
    APU reset; pulse(reset); reset.write(0xFF11,0xBF);reset.write(0xFF14,0xC6);
    reset.update(4096); reset.resetDivider(); assert(!(reset.read(0xFF26)&1));

    APU sweep; pulse(sweep); sweep.write(0xFF10,0x11);
    sweep.write(0xFF13,0xE8);sweep.write(0xFF14,0x83); // 1000 -> 1500 -> overflow check
    assert(sweep.read(0xFF26)&1);sweep.update(24576);assert(!(sweep.read(0xFF26)&1));

    APU second;second.write(0xFF25,0x22);second.write(0xFF16,0x80);
    second.write(0xFF17,0x21);second.write(0xFF18,0xD6);second.write(0xFF19,0x86);
    assert(second.read(0xFF26)&2); second.update(4194304/4);second.takeSamples();
    second.update(4194304/4);assert(peak(second.takeSamples())<0.0001);
    assert(second.read(0xFF26)&2); // Zero envelope volume doesn't clear status.

    APU wave; wave.write(0xFF25,0x44);
    for(int i=0;i<16;++i) wave.write(0xFF30+i,i<8?0xFF:0);
    wave.write(0xFF1A,0x80);wave.write(0xFF1C,0x20);
    wave.write(0xFF1D,0xD6);wave.write(0xFF1E,0x86);
    wave.update(4194304);auto w=wave.takeSamples();
    assert((wave.read(0xFF26)&4) && rises(w)>210 && rises(w)<230 && peak(w)>0.05);
    wave.write(0xFF26,0);assert(wave.read(0xFF30)==0xFF && wave.read(0xFF26)==0x70);

    APU noise;noise.write(0xFF25,0x88);noise.write(0xFF21,0xF0);
    noise.write(0xFF22,0x35);noise.write(0xFF23,0x80);
    noise.update(100000);auto n=noise.takeSamples();
    assert((noise.read(0xFF26)&8) && peak(n)>0.05 && rises(n)>5);
    noise.write(0xFF22,0x3D);noise.write(0xFF23,0x80);
    noise.update(100000);assert(noise.takeSamples()!=n);
    APU bounded;pulse(bounded);bounded.update(4194304*2);
    assert(bounded.takeSamples().size()==96000);
#ifdef GB_WITH_SDL
    AudioOutput device;
    assert(device.open()); assert(device.queue(tone)); assert(device.queue(left));
#endif
    std::cout<<"Audio pitch, sample rate, routing, channels, length, sweep, envelope, and power tests passed\n";
}
