#include "nes.h"

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

u8 default_cartridge_read(u16 adr)
{
    if(adr >= 0x8000 && adr < 0xC000) // PRG-ROM
    {
        return emu.currentPrg1Ptr[adr&0x3FFF];
    }
    else if(adr >= 0xC000)
    {
        return emu.currentPrg2Ptr[adr&0x3FFF];
    }
    return 0;
}

void default_cartridge_write(u16 adr, u8 val)
{
}

u8 shiftRegister;
u8 shiftCount;
u8 chrBankMode;
u8 prgBankMode;

void mapper001_write(u16 adr, u8 val)
{
    u8 reset = (val&0x80)!=0;
    if(reset)
    {
        //printf("reset\n");
        shiftRegister = 0;
        shiftCount = 0;
    }
    else
    {
        shiftRegister >>= 1;
        shiftRegister |= (val&1)<<4;
        shiftCount++;
        if(shiftCount == 5) // 5th write
        {
            u8 high3 = (adr>>12)&(~1);
            switch(high3)
            {
                case 0x8: // control
                    {
                        u8 mirroring = shiftRegister&0x3;
                        u8 prgMode = (shiftRegister>>2)&0x3;
                        switch(mirroring)
                        {
                            case 0: emu_set_nt_mirroring(NAMETABLE_MIRRORING_SINGLE_LOW); break;
                            case 1: emu_set_nt_mirroring(NAMETABLE_MIRRORING_SINGLE_HIGH); break;
                            case 2: emu_set_nt_mirroring(NAMETABLE_MIRRORING_VERTICAL); break;
                            case 3: emu_set_nt_mirroring(NAMETABLE_MIRRORING_HORIZONTAL); break;
                            default: break;
                        }
                        switch(prgMode)
                        {
                            case 0:
                            case 1:
                                prgBankMode = 0;
                                printf("prg bank mode: switch 32k\n");
                                break;
                            case 2:
                                prgBankMode = 2;
                                printf("prg bank mode: fix 0x8000\n");
                                break;
                            case 3:
                                prgBankMode = 3;
                                printf("prg bank mode: fix 0xC000\n");
                                break;
                            default: break;
                        }
                        if((shiftRegister&0x10) != 0) // chr bank 4k
                            chrBankMode = 1;
                        else // chr bank 8k
                            chrBankMode = 0;
                    }
                    break;
                case 0xA: // chr 0
                    //printf("CHR0 to %02d %02d\n", shiftRegister>>1, shiftRegister);
                    if(chrBankMode == 0)
                    {
                        assert((shiftRegister>>1) < emu.chrRomBlockCount);
                        emu.currentChrLowPtr = emu.chrRomBlocks[shiftRegister>>1];
                        emu.currentChrHiPtr = emu.chrRomBlocks[shiftRegister>>1]+4096;
                    }
                    else
                    {
                        assert((shiftRegister>>1) < emu.chrRomBlockCount);
                        u32 add = (shiftRegister&0x1)?4096:0;
                        emu.currentChrLowPtr = emu.chrRomBlocks[shiftRegister>>1]+add;
                    }
                    break;
                case 0xC: // chr 1
                    if(chrBankMode == 1)
                    {
                        //printf("CHR1 to %02d %02d\n", shiftRegister>>1, shiftRegister);
                        assert((shiftRegister>>1) < emu.chrRomBlockCount);
                        u32 add = (shiftRegister&0x1)?4096:0;
                        emu.currentChrHiPtr = emu.chrRomBlocks[shiftRegister>>1]+add;
                    }
                    break;
                case 0xE: // prg
                    printf("PRG to %04d / %04d\n", shiftRegister, emu.prgRamBlockCount);
                    switch(prgBankMode)
                    {
                        case 0:
                            assert((shiftRegister&0xE)+1 < emu.prgRamBlockCount);
                            emu.currentPrg1Ptr = emu.prgRamBlocks[shiftRegister&0xE];
                            emu.currentPrg2Ptr = emu.prgRamBlocks[(shiftRegister&0xE)+1];
                            break;
                        case 2:
                            assert((shiftRegister&0xF) < emu.prgRamBlockCount);
                            emu.currentPrg1Ptr = emu.prgRamBlocks[0];
                            emu.currentPrg2Ptr = emu.prgRamBlocks[shiftRegister&0xF];
                            break;
                        case 3: 
                            //assert((shiftRegister&0xF) < emu.prgRamBlockCount);
                            if((shiftRegister&0xF) >= emu.prgRamBlockCount)
                                break;
                            emu.currentPrg1Ptr = emu.prgRamBlocks[shiftRegister&0xF];
                            emu.currentPrg2Ptr = emu.prgRamBlocks[emu.prgRamBlockCount-1];
                            break;
                        default: break;
                    }
                    break;
            }
            //TODO: change bank
            shiftRegister = 0;
            shiftCount = 0;
        }
    }
}

u8 mapper003_read(u16 adr)
{
    return 0;
}

void mapper003_write(u16 adr, u8 val)
{
    if(val < emu.chrRomBlockCount)
    {
        emu.currentChrLowPtr = emu.chrRomBlocks[val];
        emu.currentChrHiPtr = emu.chrRomBlocks[val]+4096;
    }
    else
        assert(false);
}

b32 mapper_init(u32 mapperId)
{
    emu.crRead = default_cartridge_read;
    emu.crWrite = default_cartridge_write;
    switch(mapperId)
    {
        case 0:
            break;
        case 1:
            if(emu.prgRamBlockCount > 16)
            {
                show_error("Error", "TODO: Highest CHR line switches 256KB PRG banks (1)");
                exit(-1);
            }
            
            emu.crWrite = mapper001_write;
            break;
        case 3:
            emu.crWrite = mapper003_write;
            break;
        default:
            return 0;
    }
    return 1;
}

