#include "nes.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

u32 displayBuffer[240][256];
u8 memory[1024*1024];

u32 frame = 0;
CPU cpu;
PPU ppu;
APU apu;
Emulator emu;

void nes_init()
{
    // TODO: read ROM
    char * buffer = 0;
    long length;
    FILE * f = fopen ("rom.nes", "rb");
    printf("Reading ROM from rom.nes\n");

    if (f)
    {
        fseek (f, 0, SEEK_END);
        length = ftell (f);
        fseek (f, 0, SEEK_SET);
        buffer = (char*)malloc (length);
        if (buffer)
        {
            fread (buffer, 1, length, f);
        }
        fclose (f);
    }

    if (buffer)
    {
        // Check if header matches "NES" + DOS EOF
        if(buffer[0] != 'N' || buffer[1] != 'E' || buffer[2] != 'S' || buffer[3] != 0x1A)
        {
            // TODO: show error and let user pick another file
            exit(-1);
        }

        u8 prgSize = buffer[4]; // number of 16k units
        u8 chrSize = buffer[5]; // number of 8k units
        u8 f6 = buffer[6];
        u8 f7 = buffer[7];
        u8 prgRAMSize = buffer[8]; // 0 means 8k, number of 8k units
        u8 f9 = buffer[9];
        u8 f10 = buffer[10];
        bool hasTrainer = (0x04&f6)!=0;
        u8 mapperNumber = (f7&0xF0) | (f6 >> 4);
        bool isPal = (0x01&f9)!=0;

        emu.tvSystem = isPal ? TV_SYSTEM_PAL : TV_SYSTEM_NTSC;
        cpu.clockHz = isPal ? 1662607 : 1789773;
        apu.clocksPerFrame = cpu.clockHz/480; // 240Hz
        printf("clocks per frame %d\n", apu.clocksPerFrame);

        cpu.decimalFlag = 1; // set bit 5 to 1

        printf("PRG ROM %dB\n", prgSize*(1<<14));
        printf("PRG RAM %dB\n", prgRAMSize*(1<<13));
        printf("CHR ROM %dB (%d blocks)\n", chrSize*(1<<13), chrSize);
        printf("hastrainer: %d\nmapperNumber: %d \nTV:%s\n",hasTrainer, mapperNumber, isPal?"PAL":"NTSC");

        // TODO: support trainer
        assert(!hasTrainer);
        // TODO: support 8k and more advanced mappings
        assert(prgSize == 2 || prgSize == 1);
        u32 start = 16;
        for(int i = 0; i < 2; i++)
        {
            if(i == 0)
            {
                u32 adr = 0x8000;
                for(int j = 0; j < 16384; j++)
                {
                    memory[adr+j] = buffer[start+j];
                }
                if(prgSize == 2)
                    start += 16384;
            }
            else if(i==1) // TODO: mirror, don't copy
            {
                u32 adr = 0xC000;
                int j;
                for(j = 0; j < 16384; j++)
                {
                    memory[adr+j] = buffer[start+j];
                }
                start+= 16384;
            }
        }

        assert(chrSize > 0);
        emu.chrRomBlocks = new u8*[chrSize];
        for(int i = 0; i < chrSize; i++)
        {
            emu.chrRomBlocks[i] = new u8[8192];
            for(int j = 0; j < 8192; j++)
            {
                emu.chrRomBlocks[i][j] = buffer[start++];
            }
        }
        emu.currentChrLowPtr = emu.chrRomBlocks[0];
        emu.currentChrHiPtr = &emu.chrRomBlocks[0][4096];

        /*for(int i = 0; i < 1; i++)
        {
            u32 adr = 0x0000;
            for(int j = 0; j < 8192; j++)
            {
                ppu_memory[adr+j] = buffer[start+j];
            }
            start += 8192;
        }*/

    }
    else
    {
        printf("Loading ROM failed\n");
        assert(false);
    }
    free(buffer);
    
    u8 pcLow = read_byte(0xFFFC);
    u8 pcHigh = read_byte(0xFFFD);
    cpu.PC = (pcHigh << 8) | pcLow;
    printf("PC set to 0x%02x\n", cpu.PC);
    cpu.S = 0;
}

void nes_frame()
{  
    u32 i; 
    for(i = 0; ppu_cycle() == false; i++)
    {
        if(i%3 == 0) // cpu runs at 3x lower clock than ppu
        {
            cpu_cycle();
            if((i%6) == 0) // apu runs every second cpu clock
                apu_cycle();
        }
    }
}
#include "nes.h"

#include <assert.h>
#include <stdio.h>
