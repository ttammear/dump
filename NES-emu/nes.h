#pragma once 

#include "ttTypes.h"

#define TV_SYSTEM_NTSC  0
#define TV_SYSTEM_PAL   1

#define SAMPLES_PER_SECOND  44100

#define NAMETABLE_MIRRORING_VERTICAL        1
#define NAMETABLE_MIRRORING_HORIZONTAL      2
#define NAMETABLE_MIRRORING_SINGLE_LOW      3
#define NAMETABLE_MIRRORING_SINGLE_HIGH     4

typedef u8   (*cartridgeReadFunc)(u16);
typedef void (*cartridgeWriteFunc)(u16, u8);

typedef struct Emulator
{
    u32 tvSystem;
    r64 secondsSinceStart;
    u32 ntMirroring;
    
    u8 **prgRamBlocks;
    u32 prgRamBlockCount;
    u8* currentPrg1Ptr;
    u8* currentPrg2Ptr;

    u8 **chrRomBlocks;
    u32 chrRomBlockCount;
    u8* currentChrLowPtr;
    u8* currentChrHiPtr;

    u8* currentNtPtr[4];

    cartridgeReadFunc crRead;
    cartridgeWriteFunc crWrite;
} Emulator;

typedef struct CPU
{
    // computational state
    u8 A; // accumulator
    u8 X; // index 1
    u8 Y; // index 2
    u16 PC; // program counter
    u8 S; // stack pointer
    //u8 P; // status register

    u8 carryFlag;
    u8 zeroFlag;
    u8 intFlag;
    u8 decimalFlag;
    u8 overflowFlag;
    u8 signFlag;

    // emulation state
    u8 strobe;
    u8 pad1Data;
    u8 pad2Data;

    u32 clockHz;
    u32 suspClocks; // clocks left to complete instruction

} __attribute__((packed)) CPU;

typedef struct Eval_Sprite
{
    u8 yPos;
    u8 idx;
    u8 attr;
    u8 xPos;
    u8 isSprite0;
    u8 isFirstPart; // false if second part of 8x16 sprite
} Eval_Sprite;

typedef struct Scanline_Data
{
    u32 palIdx;
    u8 indexBits;
    u8 lowSR;
    u8 highSR;
    Eval_Sprite sprites[8];
    u32 spriteCount;
} Scanline_Data;

typedef struct PPU
{
    u8 CTRL;
    u8 MASK;
    u8 STATUS;
    u8 OAM_ADDR;

    u8 OAM_DATA;
    u8 SCROLL;
    u8 ADDR;
    u8 DATA;

    u8 OAM_DMA; // last register
    u8 latchToggle;
    u8 scrollX;
    u8 scrollY;

    u8 bufferedData;
    u8 busLatch;
    u8 align1[2];

    u16 accessAdr;
    u16 nameTblAdr;
    u16 spriteTblAdr;
    u16 bgTblAdr;

    i32 scanline;
    u32 clk;

    Scanline_Data slData;

} __attribute__((packed)) PPU;

typedef struct SweepUnit
{
    u8 reload;
    u8 divider;
} SweepUnit;

typedef struct EnvelopeUnit
{
    u8 startFlag;
    u8 divider;
    u8 decay;
} EnvelopeUnit;

typedef struct APU
{
    u8 PULSE1[4];       // 0x4000
    u8 PULSE2[4];       // 0x4004
    u8 TRIANGLE[4];     // 0x4008
    u8 NOISE[4];        // 0x400C
    u8 DMC_CHANNEL_0;   // 0x4010
    u8 DMC_CHANNEL_1;
    u8 DMC_CHANNEL_2;
    u8 DMC_CHANNEL_3;   // 0x4013
    u8 NA1;
    u8 CONTROL;         // 0x4015
    u8 NA2;
    u8 FRAME_COUNTER;   // 0x4017

    // emulation variables
    u8 p1Length;
    u8 p2Length;

    EnvelopeUnit p1Env;
    EnvelopeUnit p2Env;
    EnvelopeUnit nEnv;

    SweepUnit p1Sweep;
    SweepUnit p2Sweep;

    // triangle channel
    u8 tLength;
    u8 tLinCounter;
    u8 tLinReloadFlag;
    u8 tLinCtrlFlag;

    u8 nLength;
    u16 nShift;
    u16 nState;

    u32 clocksPerFrame;
    // keeps track of how many apu clocks until next frame counter tick
    u32 frameClkCount;
    // keeps track of the 4 or 5 frame counter tick sequence
    u32 frameCounter;
} __attribute__((packed)) APU;

extern Emulator emu;
extern CPU cpu;
extern PPU ppu;
extern APU apu;
extern u32 displayBuffer[240][256];
extern u8 memory[];
extern u32 palette[64];
extern u8 ppu_spr_ram[];
extern u8 ppu_internal_ram[2048];
extern u8 buttons;

void AudioCallback(void *userdata, u8 *stream, i32 len);
void show_error(const char* title, const char* message);
void nes_init();
void nes_frame();
void nes_cleanup();
b32 mapper_init(u32 mapperId);
void emu_set_nt_mirroring(u32 mode);
u8 read_byte(u16 adr);
void write_byte(u16 adr, u8 value);
u8 ppu_read_byte(u16 adr);
void ppu_write_byte(u16 adr, u8 value);
u8 ppu_get_register(u8 reg);
void ppu_set_register(u8 reg, u8 value);
void apu_set_register(u8 reg, u8 value);
void apu_init();
void cpu_cycle();
b32 ppu_cycle();
void apu_cycle();
void cpu_nmi_trigger();
u8 ppu_read_oam_data(u8 adr);
