#include "nes.h"

#include <math.h>
#include <assert.h>
#include <stdio.h>
#include <time.h>

static void apu_half_frame();
static void apu_quarter_frame();

u8 lengthTable[32] =
{
    10,254, 20,  2, 40,  4, 80,  6, 160,  8, 60, 10, 14, 12, 26, 14,
    12, 16, 24, 18, 48, 20, 96, 22, 192, 24, 72, 26, 16, 28, 32, 30
};

u8 pulseDuty[4][8] = 
{
    { 0, 1, 0, 0, 0, 0, 0, 0 },
    { 0, 1, 1, 0, 0, 0, 0, 0 },
    { 0, 1, 1, 1, 1, 0, 0, 0 },
    { 1, 0, 0, 1, 1, 1, 1, 1 }
};

u8 triangleTable[32] =
{
    15, 14, 13, 12, 11, 10,  9,  8,  7,  6,  5,  4,  3,  2,  1,  0,
     0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15
};

u16 ntscNoisePeriod[16] =  {
    4, 8, 16, 32, 64, 96, 128, 160, 202, 254, 380, 508, 762, 1016, 2034, 4068
};

u16 palNoisePeriod[16] = {
    4, 8, 14, 30, 60, 88, 118, 148, 188, 236, 354, 472, 708,  944, 1890, 3778
};

u32 started;

r64 p1AmpProg = 0.0;
r64 p2AmpProg = 0.0;
r64 tAmpProg = 0.0;
r64 nAmpProg = 0.0;

// callback from platform layer asking for more audio
void AudioCallback(void *userdata, u8 *stream, i32 len)
{
    if(!started)
        return;

    i16* stream16 = (i16*)stream;
    u32 len16 = len/2;

    u8 p1Ctrl = apu.PULSE1[0];
    u8 p2Ctrl = apu.PULSE2[0];
    u8 nCtrl = apu.NOISE[0];
    u16 p1Vol = (p1Ctrl & 0x10) == 0 ? apu.p1Env.decay * 600 : (p1Ctrl & 0x0F) * 600;
    u16 p2Vol = (p2Ctrl & 0x10) == 0 ? apu.p2Env.decay * 600 : (p2Ctrl & 0x0F) * 600;
    u16 nVol = (nCtrl & 0x10) == 0 ? apu.nEnv.decay*2 : (nCtrl & 0x0F)*2;
    
    u16 p1T = apu.PULSE1[2] | ((apu.PULSE1[3]&0x7)<<8); // timer value
    u32 p1Freq = cpu.clockHz / (16*(p1T+1)); // fequency
    u32 p1P = cpu.clockHz/(16*p1Freq) - 1;

    u16 p2T = apu.PULSE2[2] | ((apu.PULSE2[3]&0x7)<<8); // timer value
    u32 p2Freq = cpu.clockHz / (16*(p2T+1)); // fequency
    u32 p2P = cpu.clockHz/(16*p2Freq) - 1;

    u16 tT = apu.TRIANGLE[2] | ((apu.TRIANGLE[3]&0x7)<<8); // timer value
    u32 tF = cpu.clockHz / (32*(tT+1)); // fequency

    u16 nT;
    if(emu.tvSystem == TV_SYSTEM_NTSC)
        nT = ntscNoisePeriod[apu.NOISE[2]&0xF];
    else
        nT = palNoisePeriod[apu.NOISE[2]&0xF];
    u32 nF = cpu.clockHz / (16*(nT+1));
    
    u32 p1Enabled = p1P >= 8 && apu.p1Length != 0 && (apu.CONTROL & 1) != 0;
    u32 p2Enabled = p2P >= 8 && apu.p2Length != 0 && (apu.CONTROL & 2) != 0;
    u32 tEnabled = apu.tLinCounter > 0 && apu.tLength > 0; 
    u32 nEnabled = apu.nLength != 0; // TODO: apu.CONTROL & 0x4?
    // TODO: DMC channel

    if((apu.CONTROL & 0x1) != 0) // pulse 1 enabled
    {
        r64 p1Amp = 1.0 / p1Freq;
        r64 p2Amp = 1.0 / p2Freq;
        r64 tAmp = 1.0 / tF;
        r64 nAmp = 1.0 / nF;

        r64 sAmp = 1.0 / SAMPLES_PER_SECOND;
        for(int i = 0; i < len16; i++)
        {
            p1AmpProg += sAmp/p1Amp;
            // modulus 0.999 because (unlikely) 1.0 would overflow duty index
            p1AmpProg = fmod(p1AmpProg, 0.999); 

            p2AmpProg += sAmp/p2Amp;
            p2AmpProg = fmod(p2AmpProg, 0.999);
            
            tAmpProg += sAmp/tAmp;
            tAmpProg = fmod(tAmpProg, 0.999);

            nAmpProg += sAmp/nAmp;
            if(nAmpProg/0.999 >= 0.999)
            {
                u8 otherBit;
                if((apu.NOISE[2]&0x80) != 0)
                    otherBit = (apu.nShift&0x40)>>6;
                else
                    otherBit = (apu.nShift&0x02)>>1;
                u8 feedback = apu.nShift&0x1 ^ otherBit;
                apu.nShift >>= 1;
                if(feedback)
                    apu.nShift |= 0x4000;
                else
                    apu.nShift &= ~0x4000;
            }
            nAmpProg = fmod(nAmpProg, 0.999);

            u32 dty1Idx = (u32)(p1AmpProg * 8);
            u32 dty2Idx = (u32)(p2AmpProg * 8);
            u32 triIdx = (u32)(tAmpProg * 32);

            // TODO: proper mixing and volume
            stream16[i] = 0;
            if(p1Enabled) // PULSE1
                stream16[i] += pulseDuty[p1Ctrl >> 6][dty1Idx]*p1Vol;
            if(p2Enabled) // PUSLE2
                stream16[i] += pulseDuty[p2Ctrl >> 6][dty2Idx]*p2Vol;
            if(tEnabled) // TRIANGLE
                stream16[i] += (triangleTable[triIdx]-7)*1200;
            if(nEnabled) // NOISE
                // TODO: replace totally random division
                stream16[i] += (apu.nShift/86)*nVol;
        }
    }
}

void apu_set_register(u8 reg, u8 value)
{
    switch(reg)
    {
        case 0x01:
        {
            apu.p1Sweep.reload = 1;
            apu.PULSE1[1] = value;
        } break;
        case 0x03: // pulse1 length/timerhigh
        {
            if((apu.CONTROL & 0x1) != 0) // pulse1 enabled
                apu.p1Length = lengthTable[value>>3];
            apu.p1Env.startFlag = 1;
            // TODO: restart envelope
            // TODO: reset phase of pulse generator (duty?)
            apu.PULSE1[3] = value;
        } break;
        case 0x05:
        {
            apu.p2Sweep.reload = 1;
            apu.PULSE2[1] = value;
        } break;
        case 0x07: // pulse2 length/timerhigh
        {
            if((apu.CONTROL & 0x2) != 0) // pulse2 enabled
                apu.p2Length = lengthTable[value>>3];
            apu.p2Env.startFlag = 1;
            // TODO: restart envelope
            // TODO: reset phase of pulse generator (duty?)
            apu.PULSE2[3] = value;
        } break;
        case 0x0B: // triangle length/timerhigh
        {
           // TODO: should this if be here?
           if((apu.CONTROL & 0x4) != 0)
               apu.tLength = lengthTable[value>>3];
            apu.tLinReloadFlag = 1; 
            apu.TRIANGLE[3] = value;
        } break;
        case 0x0F: // noise length
        {
            if((apu.CONTROL & 0x8) != 0)
                apu.nLength = lengthTable[value>>3];
            apu.nEnv.startFlag = 1;
            apu.NOISE[3] = value;
        } break;
        case 0x15: // APU CTRL
        {
            if((value & 0x1) == 0) // p1 enable bit
                apu.p1Length = 0;
            if((value & 0x2) == 0) // p2 enable bit
                apu.p2Length = 0;
            apu.CONTROL = value;
        } break;
        case 0x17: // FRAME_COUNTER 
        {
            // TODO: reset frame counter on write
            // with bit 7 set ($80) will immediately clock all of its controlled units at the beginning of the 5-step sequence (wtf does this even mean)
            apu.FRAME_COUNTER = value;
            apu.frameClkCount = 0;
            apu.frameCounter = 0;
            if((apu.FRAME_COUNTER & 0x80) != 0)
            {
                apu_half_frame();
                apu_quarter_frame();
            }
        } break;
        default:
        {
            *((u8*)&apu + reg) = value;
        } break;
    }
}

static inline void apu_length_tick()
{
    if((apu.PULSE1[0] & 0x20) == 0 && apu.p1Length > 0) // length halt bit clear
        apu.p1Length--;
    if((apu.PULSE2[0] & 0x20) == 0 && apu.p2Length > 0)
        apu.p2Length--;
    if((apu.TRIANGLE[0] & 0x80) == 0 && apu.tLength > 0)
        apu.tLength--;
    if((apu.NOISE[0] & 0x20) == 0 && apu.nLength > 0)
        apu.nLength--;
}

static inline void sweep(SweepUnit *su, u8 chReg[], u32 channel)
{
    u16 shiftAmount = ((chReg[1])&0x7);
    u16 timer = (chReg[2] | ((chReg[3]&0x07)<<8));
    u16 changeAmount = timer >> shiftAmount;
    u16 targetPeriod;
    if((chReg[1] & 0x08) != 0)
    {
        targetPeriod = timer - changeAmount;
        if(channel == 1)
            targetPeriod--;
    }
    else
    {
        targetPeriod = timer + changeAmount;
    }
    if(targetPeriod <= 0x7FF && targetPeriod >= 8)
    {
        chReg[2] = targetPeriod & 0xFF;
        chReg[3] = (chReg[3] & ~0x7) | ((targetPeriod >> 8) & 0x7);
    } // TODO: else send 0 to mixer
}

static inline void sweep_tick(SweepUnit *su, u8 chReg[], u32 channel)
{
    u8 sweepCtrl = chReg[1];
    if(su->reload)
    {
        if(su->divider == 0 && (sweepCtrl & 0x80) != 0)
        {
            sweep(su, chReg, channel);
        }
        su->divider = (sweepCtrl >> 4) & 0x7;
        su->reload = 0;
    }
    else if(su->divider != 0)
        su->divider--;
    else
    {
        if((sweepCtrl & 0x80) != 0)
        {
            su->divider = (sweepCtrl >> 4) & 0x7;
            sweep(su, chReg, channel);
        }
    }
}

static inline void envelope_tick(EnvelopeUnit *env, u8 ctrlReg)
{
    if(!env->startFlag) // start flag clear, clock divider
    {
        if(env->divider == 0)
        {
            env->divider = ctrlReg&0x0F;
            if(env->decay != 0)
                env->decay--;
            else if((ctrlReg & 0x20) != 0) // looping enabled
                env->decay = 15;
        }
        else
            env->divider--;
    }
    else // start envelope divider
    {
        env->divider = ctrlReg&0x0F;
        env->decay = 15;
        env->startFlag = 0;
    }
}

// triangle channel linear counter
static inline void apu_linear_counter_tick()
{
    if(apu.tLinReloadFlag)
        apu.tLinCounter = apu.TRIANGLE[0] & 0x7F;
    else
    {
        if(apu.tLinCounter != 0)
            apu.tLinCounter--;
    }
    if((apu.TRIANGLE[0] & 0x80) == 0) // tri ctrl flag clear
        apu.tLinReloadFlag = 0;
}

static inline void apu_half_frame()
{
    apu_length_tick();
    sweep_tick(&apu.p1Sweep, apu.PULSE1, 1);
    sweep_tick(&apu.p2Sweep, apu.PULSE2, 2);
}

static inline void apu_quarter_frame()
{
    envelope_tick(&apu.p1Env, apu.PULSE1[0]);
    envelope_tick(&apu.p2Env, apu.PULSE2[0]);
    envelope_tick(&apu.nEnv, apu.NOISE[0]);
    apu_linear_counter_tick();
}

void apu_init()
{
    apu.nShift = 1;
}

// APU clock cycle at (CPU clock / 2)
void apu_cycle()
{
    // TODO: if CPU interrupt inhibit is set, don't set FC interrupt flag
    started = 1;

    if(apu.frameClkCount >= apu.clocksPerFrame) // 240Hz frame counter
    {
        if((apu.FRAME_COUNTER & 0x80) == 0) // 4 step mode
        {
            if(apu.frameCounter == 1 || apu.frameCounter == 3) // sweep and length counters (~120Hz)
            {
                apu_half_frame();
            }
            // TODO: interrupt (also see comment at the start of this function)
            apu_quarter_frame(); // full ~240Hz
            if(apu.frameCounter >= 3)
                apu.frameCounter = 0;
        }
        else // 5 step mode
        {
            if(apu.frameCounter == 1 || apu.frameCounter == 4) // sweep and length counters (~96Hz)
            {
                apu_half_frame();
            }
            // TODO: interrupt
            if(apu.frameCounter != 3) // ~192Hz
                apu_quarter_frame();

            if(apu.frameCounter >= 4)
                apu.frameCounter = 0;
        }
        apu.frameCounter++;
        apu.frameClkCount = 0;
    }
    apu.frameClkCount++;
}

