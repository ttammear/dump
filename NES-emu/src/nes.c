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

b32 platform_choose_file(char *buffer, int bufSize);

void show_error(const char* title, const char* error)
{
    printf("Error: %s, Internal: %s", title, error);
}

void emu_set_nt_mirroring(u32 mirroring)
{
    switch(mirroring)
    {
        case NAMETABLE_MIRRORING_SINGLE_LOW:
            emu.currentNtPtr[0] = ppu_internal_ram;
            emu.currentNtPtr[1] = ppu_internal_ram;
            emu.currentNtPtr[2] = ppu_internal_ram;
            emu.currentNtPtr[3] = ppu_internal_ram;
            break;
        case NAMETABLE_MIRRORING_SINGLE_HIGH:
            emu.currentNtPtr[0] = ppu_internal_ram+0x400;
            emu.currentNtPtr[1] = ppu_internal_ram+0x400;
            emu.currentNtPtr[2] = ppu_internal_ram+0x400;
            emu.currentNtPtr[3] = ppu_internal_ram+0x400;
            break;
        case NAMETABLE_MIRRORING_VERTICAL:
            emu.currentNtPtr[0] = ppu_internal_ram;
            emu.currentNtPtr[1] = ppu_internal_ram+0x400;
            emu.currentNtPtr[2] = ppu_internal_ram;
            emu.currentNtPtr[3] = ppu_internal_ram+0x400;
            break;
        case NAMETABLE_MIRRORING_HORIZONTAL:
            emu.currentNtPtr[0] = ppu_internal_ram;
            emu.currentNtPtr[1] = ppu_internal_ram;
            emu.currentNtPtr[2] = ppu_internal_ram+0x400;
            emu.currentNtPtr[3] = ppu_internal_ram+0x400;
            break;
        default:
            show_error("Error", "Invalid nametable mirroring requested");
            break;
    }
}

void nes_init()
{
    //char fileNameBuffer[4096];
    //platform_choose_file(fileNameBuffer, 4096);
    char *fileNameBuffer = "Dr. Mario (Japan, USA).nes";

    // TODO: read ROM
    char * buffer = 0;
    long length;
    FILE * f = fopen (fileNameBuffer, "rb");
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
    else
    {
        printf("fopen failed\n");
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
        b32 hasTrainer = (0x04&f6)!=0;
        b32 ntMirroring = f6&1; // 0 = horizontal, 1 = vertical
        u8 mapperNumber = (f7&0xF0) | (f6 >> 4);
        b32 isPal = (0x01&f9)!=0;

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
        assert(prgSize > 0);
        u32 start = 16;

        // TODO: free
        emu.prgRamBlocks = malloc(prgSize);
        emu.prgRamBlockCount = prgSize;

        for(int i = 0; i < prgSize; i++)
        {
            // TODO: free
            emu.prgRamBlocks[i] = malloc(16384);
            for(int j = 0; j < 0x4000; j++) // 16K
            {
                emu.prgRamBlocks[i][j] = buffer[start++];
            }
        }

        if(prgSize == 1)
        {
            emu.currentPrg1Ptr = emu.prgRamBlocks[0];
            emu.currentPrg2Ptr = emu.prgRamBlocks[0];
        }
        else
        {
            emu.currentPrg1Ptr = emu.prgRamBlocks[0];
            emu.currentPrg2Ptr = emu.prgRamBlocks[emu.prgRamBlockCount-1];
        }

        assert(chrSize > 0);
        // TODO: free
        emu.chrRomBlocks = malloc(chrSize * sizeof(u8*));
        for(int i = 0; i < chrSize; i++)
        {
            // TODO: free
            emu.chrRomBlocks[i] = malloc(8192);
            for(int j = 0; j < 8192; j++)
            {
                emu.chrRomBlocks[i][j] = buffer[start++];
            }
        }
        emu.currentChrLowPtr = emu.chrRomBlocks[0];
        emu.currentChrHiPtr = &emu.chrRomBlocks[0][4096];
        emu.chrRomBlockCount = chrSize;

        emu_set_nt_mirroring(ntMirroring ? NAMETABLE_MIRRORING_VERTICAL : NAMETABLE_MIRRORING_HORIZONTAL);
        mapper_init(mapperNumber);
    }
    else
    {
        printf("Loading ROM failed\n");
        assert(FALSE);
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
    for(i = 0; ppu_cycle() == FALSE; i++)
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
