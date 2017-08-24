#include "nes.h"

#include <stdio.h>
#include <assert.h>
#include <stdarg.h>

#define OP_STP 0x01
#define OP_NOP 0x02

#define OP_LDY 0x11
#define OP_JMP 0x12
#define OP_CLI 0x13
#define OP_SEI 0x14
#define OP_BIT 0x15
#define OP_CPX 0x16
#define OP_CPY 0x17
#define OP_PHP 0x18
#define OP_PLP 0x19
#define OP_DEY 0x1A
#define OP_TAY 0x1B
#define OP_INY 0x1C
#define OP_TYA 0x1D
#define OP_STY 0x1E
#define OP_PLA 0x1F
#define OP_RTS 0x20
#define OP_BVC 0x21
#define OP_PHA 0x22
#define OP_RTI 0x23
#define OP_SEC 0x24
#define OP_BMI 0x25
#define OP_JSR 0x26
#define OP_CLC 0x27
#define OP_BPL 0x28
#define OP_BRK 0x29
#define OP_BVS 0x2A
#define OP_BCC 0x2B
#define OP_SHY 0x2C
#define OP_BCS 0x2D
#define OP_CLV 0x2E
#define OP_BNE 0x2F
#define OP_CLD 0x30
#define OP_INX 0x31
#define OP_BEQ 0x32
#define OP_SED 0x33

#define OP_EOR 0x41
#define OP_AND 0x42
#define OP_ORA 0x43
#define OP_ADC 0x44
#define OP_STA 0x45
#define OP_LDA 0x46
#define OP_CMP 0x47
#define OP_SBC 0x48

#define OP_INC 0x60
#define OP_DEC 0x61
#define OP_LDX 0x62
#define OP_TSX 0x63
#define OP_TXS 0x64
#define OP_TXA 0x65
#define OP_ROR 0x66
#define OP_STX 0x67
#define OP_LSR 0x68
#define OP_ROL 0x69
#define OP_ASL 0x6A
#define OP_SHX 0x6B
#define OP_TAX 0x6C
#define OP_DEX 0x6D // 59 official

#define OP_DCP 0x71
#define OP_LAS 0x72
#define OP_LAX 0x73
#define OP_SAX 0x74
#define OP_RRA 0x75
#define OP_SRE 0x76
#define OP_RLA 0x77
#define OP_SLO 0x78
#define OP_ISC 0x79
#define OP_AHX 0x7A
#define OP_ARR 0x7B
#define OP_ALR 0x7C
#define OP_ANC 0x7D
#define OP_XAA 0x7E
#define OP_TAS 0x7F
#define OP_AXS 0x70 // 75 with unofficial

#define ADR_MODE_IMPLIED        0x0
#define ADR_MODE_IMMEDIATE      0x1
#define ADR_MODE_INDEXED_INDIR  0x2
#define ADR_MODE_ZP             0x3
#define ADR_MODE_ABS            0x4
#define ADR_MODE_INDIR_INDEXED  0x5
#define ADR_MODE_ZP_INDEXED_X   0x6
#define ADR_MODE_ZP_INDEXED_Y   0x7
#define ADR_MODE_ABS_INDEXED_X  0x8
#define ADR_MODE_ABS_INDEXED_Y  0x9
#define ADR_MODE_PC_RELATIVE    0xA
#define ADR_MODE_JMP_INDIR      0xB

#define FETCH_PC_BYTE() (read_byte(cpu.PC++))

#define SET_SIGN_FLAG_VAL(x) (cpu.signFlag = ((x)&0x80)!=0)
#define SET_ZERO_FLAG_VAL(x) (cpu.zeroFlag = ((x)==0))

static inline void PUSH_BYTE(u8 b)
{
    u16 adr = 0x100 | cpu.S;
    write_byte(adr, b);
    cpu.S--;
}

static inline u8 POP_BYTE()
{
    u8 prev = ++cpu.S;
    u16 adr = 0x100|prev;
    u8 b = read_byte(adr);
    return b;
}

const char* adr_mode_to_string(u8 adrMode)
{
    switch(adrMode)
    {
    case ADR_MODE_IMPLIED:
        return "ADR_MODE_IMPLIED";
    case ADR_MODE_IMMEDIATE:
        return "ADR_MODE_IMMEDIATE";
    case ADR_MODE_INDEXED_INDIR:
        return "ADR_MODE_INDEXED_INDIR";
    case ADR_MODE_ZP:
        return "ADR_MODE_ZP";
    case ADR_MODE_ABS:
        return "ADR_MODE_ABS";
    case ADR_MODE_INDIR_INDEXED:
        return "ADR_MODE_INDIR_INDEXED";
    case ADR_MODE_ZP_INDEXED_X:
        return "ADR_MODE_ZP_INDEXED_X";
    case ADR_MODE_ZP_INDEXED_Y:
        return "ADR_MODE_ZP_INDEXED_Y";
    case ADR_MODE_ABS_INDEXED_X:
        return "ADR_MODE_ABS_INDEXED_X";
    case ADR_MODE_ABS_INDEXED_Y:
        return "ADR_MODE_ABS_INDEXED_Y";
    case ADR_MODE_PC_RELATIVE:
        return "ADR_MODE_PC_RELATIVE";
    case ADR_MODE_JMP_INDIR:
        return "ADR_MODE_JMP_INDIR";
    default:
        return "ADR_MODE_UNKNOWN";
    }
}

const char* inst_to_string(u8 op)
{
    switch(op)
    {
        case OP_STP:
            return "STP";
        case OP_NOP:
            return "NOP";
        case OP_LDY:
            return "LDY";
        case OP_JMP:
            return "JMP";
        case OP_CLI:
            return "CLI";
        case OP_SEI:
            return "SEI";
        case OP_BIT:
            return "BIT";
        case OP_CPX:
            return "CPX";
        case OP_CPY:
            return "CPY";
        case OP_PHP:
            return "PHP";
        case OP_PLP:
            return "PLP";
        case OP_DEY:
            return "DEY";
        case OP_TAY:
            return "TAY";
        case OP_INY:
            return "INY";
        case OP_TYA:
            return "TYA";
        case OP_STY:
            return "STY";
        case OP_PLA:
            return "PLA";
        case OP_RTS:
            return "RTS";
        case OP_BVC:
            return "BVC";
        case OP_PHA:
            return "PHA";
        case OP_RTI:
            return "RTI";
        case OP_SEC:
            return "SEC";
        case OP_BMI:
            return "BMI";
        case OP_JSR:
            return "JSR";
        case OP_CLC:
            return "CLC";
        case OP_BPL:
            return "BPL";
        case OP_BRK:
            return "BRK";
        case OP_BVS:
            return "BVS";
        case OP_BCC:
            return "BCC";
        case OP_SHY:
            return "SHY";
        case OP_BCS:
            return "BCS";
        case OP_CLV:
            return "CLV";
        case OP_BNE:
            return "BNE";
        case OP_CLD:
            return "CLD";
        case OP_INX:
            return "INX";
        case OP_BEQ:
            return "BEQ";
        case OP_SED:
            return "SED";

        case OP_EOR:
            return "EOR";
        case OP_AND:
            return "AND";
        case OP_ORA:
            return "ORA";
        case OP_ADC:
            return "ADC";
        case OP_STA:
            return "STA";
        case OP_LDA:
            return "LDA";
        case OP_CMP:
            return "CMP";
        case OP_SBC:
            return "SBC";
        case OP_INC:
            return "INC";
        case OP_DEC:
            return "DEC";
        case OP_LDX:
            return "LDX";
        case OP_TSX:
            return "TSX";
        case OP_TXS:
            return "TXS";
        case OP_TXA:
            return "TXA";
        case OP_ROR:
            return "ROR";
        case OP_STX:
            return "STX";
        case OP_LSR:
            return "LSR";
        case OP_ROL:
            return "ROL";
        case OP_ASL:
            return "ASL";
        case OP_SHX:
            return "SHX";
        case OP_TAX:
            return "TAX";
        case OP_DEX:
            return "DEX";
        case OP_DCP:
            return "DCP";
        case OP_LAS:
            return "LAS";
        case OP_LAX:
            return "LAX";
        case OP_SAX:
            return "SAX";
        case OP_RRA:
            return "RRA";
        case OP_SRE:
            return "SRE";
        case OP_RLA:
            return "RLA";
        case OP_SLO:
            return "SLO";
        case OP_ISC:
            return "ISC";
        case OP_AHX:
            return "AHX";
        case OP_ARR:
            return "ARR";
        case OP_ALR:
            return "ALR";
        case OP_ANC:
            return "ANC";
        case OP_XAA:
            return "XAA";
        case OP_TAS:
            return "TAS";
        case OP_AXS:
            return "AXS";
    }
    return "UNKNOWN";
}

u8 decode_table[256] = 
{
    OP_BRK, OP_ORA, OP_STP, OP_SLO, OP_NOP, OP_ORA, OP_ASL, OP_SLO, 
    OP_PHP, OP_ORA, OP_ASL, OP_ANC, OP_NOP, OP_ORA, OP_ASL, OP_SLO, 
    OP_BPL, OP_ORA, OP_STP, OP_SLO, OP_NOP, OP_ORA, OP_ASL, OP_SLO,
    OP_CLC, OP_ORA, OP_NOP, OP_SLO, OP_NOP, OP_ORA, OP_ASL, OP_SLO,
    
    OP_JSR, OP_AND, OP_STP, OP_RLA, OP_BIT, OP_AND, OP_ROL, OP_RLA, 
    OP_PLP, OP_AND, OP_ROL, OP_ANC, OP_BIT, OP_AND, OP_ROL, OP_RLA, 
    OP_BMI, OP_AND, OP_STP, OP_RLA, OP_NOP, OP_AND, OP_ROL, OP_RLA,
    OP_SEC, OP_AND, OP_NOP, OP_RLA, OP_NOP, OP_AND, OP_ROL, OP_RLA,
    
    OP_RTI, OP_EOR, OP_STP, OP_SRE, OP_NOP, OP_EOR, OP_LSR, OP_SRE, 
    OP_PHA, OP_EOR, OP_LSR, OP_ALR, OP_JMP, OP_EOR, OP_LSR, OP_SRE, 
    OP_BVC, OP_EOR, OP_STP, OP_SRE, OP_NOP, OP_EOR, OP_LSR, OP_SRE,
    OP_CLI, OP_EOR, OP_NOP, OP_SRE, OP_NOP, OP_EOR, OP_LSR, OP_SRE,
    
    OP_RTS, OP_ADC, OP_STP, OP_RRA, OP_NOP, OP_ADC, OP_ROR, OP_RRA, 
    OP_PLA, OP_ADC, OP_ROR, OP_ARR, OP_JMP, OP_ADC, OP_ROR, OP_RRA, 
    OP_BVS, OP_ADC, OP_STP, OP_RRA, OP_NOP, OP_ADC, OP_ROR, OP_RRA,
    OP_SEI, OP_ADC, OP_NOP, OP_RRA, OP_NOP, OP_ADC, OP_ROR, OP_RRA,
    
    OP_NOP, OP_STA, OP_NOP, OP_SAX, OP_STY, OP_STA, OP_STX, OP_SAX, 
    OP_DEY, OP_NOP, OP_TXA, OP_XAA, OP_STY, OP_STA, OP_STX, OP_SAX, 
    OP_BCC, OP_STA, OP_STP, OP_AHX, OP_STY, OP_STA, OP_STX, OP_SAX,
    OP_TYA, OP_STA, OP_TXS, OP_TAS, OP_SHY, OP_STA, OP_SHX, OP_AHX,
    
    OP_LDY, OP_LDA, OP_LDX, OP_LAX, OP_LDY, OP_LDA, OP_LDX, OP_LAX, 
    OP_TAY, OP_LDA, OP_TAX, OP_LAX, OP_LDY, OP_LDA, OP_LDX, OP_LAX, 
    OP_BCS, OP_LDA, OP_STP, OP_LAX, OP_LDY, OP_LDA, OP_LDX, OP_LAX,
    OP_CLV, OP_LDA, OP_TSX, OP_LAS, OP_LDY, OP_LDA, OP_LDX, OP_LAX,
    
    OP_CPY, OP_CMP, OP_NOP, OP_DCP, OP_CPY, OP_CMP, OP_DEC, OP_DCP, 
    OP_INY, OP_CMP, OP_DEX, OP_AXS, OP_CPY, OP_CMP, OP_DEC, OP_DCP, 
    OP_BNE, OP_CMP, OP_STP, OP_DCP, OP_NOP, OP_CMP, OP_DEC, OP_DCP,
    OP_CLD, OP_CMP, OP_NOP, OP_DCP, OP_NOP, OP_CMP, OP_DEC, OP_DCP,
    
    OP_CPX, OP_SBC, OP_NOP, OP_ISC, OP_CPX, OP_SBC, OP_INC, OP_ISC, 
    OP_INX, OP_SBC, OP_NOP, OP_SBC, OP_CPX, OP_SBC, OP_INC, OP_ISC, 
    OP_BEQ, OP_SBC, OP_STP, OP_ISC, OP_NOP, OP_SBC, OP_INC, OP_ISC,
    OP_SED, OP_SBC, OP_NOP, OP_ISC, OP_NOP, OP_SBC, OP_INC, OP_ISC
};


u8 adr_mode_table[256] =
{
    0,  2,  0,  2,  3,  3,  3,  3,  0,  1,  0,  1,  4,  4,  4,  4,
   10,  5,  0,  5,  6,  6,  6,  6,  0,  9,  0,  9,  8,  8,  8,  8,

    4,  2,  0,  2,  3,  3,  3,  3,  0,  1,  0,  1,  4,  4,  4,  4,
   10,  5,  0,  5,  6,  6,  6,  6,  0,  9,  0,  9,  8,  8,  8,  8,

    0,  2,  0,  2,  3,  3,  3,  3,  0,  1,  0,  1,  4,  4,  4,  4,
   10,  5,  0,  5,  6,  6,  6,  6,  0,  9,  0,  9,  8,  8,  8,  8,

    0,  2,  0,  2,  3,  3,  3,  3,  0,  1,  0,  1, 11,  4,  4,  4,
   10,  5,  0,  5,  6,  6,  6,  6,  0,  9,  0,  9,  8,  8,  8,  8,

    1,  2,  1,  2,  3,  3,  3,  3,  0,  1,  0,  1,  4,  4,  4,  4,
   10,  5,  0,  5,  6,  6,  7,  7,  0,  9,  0,  9,  8,  8,  8,  8,

    1,  2,  1,  2,  3,  3,  3,  3,  0,  1,  0,  1,  4,  4,  4,  4,
   10,  5,  0,  5,  6,  6,  7,  7,  0,  9,  0,  9,  8,  8,  9,  8,

    1,  2,  1,  2,  3,  3,  3,  3,  0,  1,  0,  1,  4,  4,  4,  4,
   10,  5,  0,  5,  6,  6,  6,  6,  0,  9,  0,  9,  8,  8,  8,  8,

    1,  2,  1,  2,  3,  3,  3,  3,  0,  1,  0,  1,  4,  4,  4,  4,
   10,  5,  0,  5,  6,  6,  6,  6,  0,  9,  0,  9,  8,  8,  8,  8
};

u8 clock_table[256] = 
{
    7, 6, 2, 8, 3, 3, 5, 5, 3, 2, 2, 2, 4, 4, 6, 6,
	2, 5, 2, 8, 4, 4, 6, 6, 2, 4, 2, 7, 4, 4, 7, 7,
	6, 6, 2, 8, 3, 3, 5, 5, 4, 2, 2, 2, 4, 4, 6, 6,
	2, 5, 2, 8, 4, 4, 6, 6, 2, 4, 2, 7, 4, 4, 7, 7,
	6, 6, 2, 8, 3, 3, 5, 5, 3, 2, 2, 2, 3, 4, 6, 6,
	2, 5, 2, 8, 4, 4, 6, 6, 2, 4, 2, 7, 4, 4, 7, 7,
	6, 6, 2, 8, 3, 3, 5, 5, 4, 2, 2, 2, 5, 4, 6, 6,
	2, 5, 2, 8, 4, 4, 6, 6, 2, 4, 2, 7, 4, 4, 7, 7,
	2, 6, 2, 6, 3, 3, 3, 3, 2, 2, 2, 2, 4, 4, 4, 4,
	2, 6, 2, 6, 4, 4, 4, 4, 2, 5, 2, 5, 5, 5, 5, 5,
	2, 6, 2, 6, 3, 3, 3, 3, 2, 2, 2, 2, 4, 4, 4, 4,
	2, 5, 2, 5, 4, 4, 4, 4, 2, 4, 2, 4, 4, 4, 4, 4,
	2, 6, 2, 8, 3, 3, 5, 5, 2, 2, 2, 2, 4, 4, 6, 6,
	2, 5, 2, 8, 4, 4, 6, 6, 2, 4, 2, 7, 4, 4, 7, 7,
	2, 6, 2, 8, 3, 3, 5, 5, 2, 2, 2, 2, 4, 4, 6, 6,
	2, 5, 2, 8, 4, 4, 6, 6, 2, 4, 2, 7, 4, 4, 7, 7
};

static u32 clk;

u8 read_byte(u16 adr)
{
    u8 ret;
    if(adr >= 0x0000 && adr < 0x2000) // internal 2KB RAM
    {
        ret = memory[adr & 0x7FF];
    }
    else if(adr >= 0x2000 && adr < 0x4000) // ppu registers
    {
        ret = ppu_get_register(adr&0x7);
    }
    else if(adr >= 0x4000 && adr < 0x4020) // APU and IO
    {
        switch(adr)
        {
            case 0x4014:
                assert(FALSE);
                break;
            case 0x4015:
                ret = apu.CONTROL;
                apu.FRAME_COUNTER &= ~0x40;
                break;
            case 0x4016: // JP 1
                ret = cpu.pad1Data;
                cpu.pad1Data <<= 1;
                ret = (ret>>7)&1;
                break;
            case 0x4018: // JP 2
                // TODO: implement
                ret = 0;
                break;
            default:
                ret = *((u8*)&apu + (adr - 0x4000));
                break;
        }
    }
    else if(adr >= 4020) // cartridge
    {
        return emu.crRead(adr);
    }
    return ret;
}

void write_byte(u16 adr, u8 value)
{
    if(adr >= 0x0000 && adr < 0x2000)
        memory[adr&0x7FF] = value;
    else if(adr >= 0x2000 && adr < 0x4000) // PPU
        ppu_set_register(adr&0x7, value);
    else if(adr >= 0x4000 && adr < 0x4020) // APU and IO registers
    {
        switch(adr)
        {
            case 0x4014: // OAM DMA
                cpu.suspClocks = 514; // suspend CPU for 514 cycles
                // write of 0xXX will copy 0xXX00-0xXXFF to PPU SPR-RAM from CPU Page
                for(u32 i = 0; i <= 0xFF; i++)
                {
                    u16 adr = (value<<8) | (i&0xFF);
                    u8 val = read_byte(adr);
                    ppu_spr_ram[i] = val;
                }
                break;
            case 0x4016: // joystick strobe
                cpu.strobe = value;
                break;
            default:
                apu_set_register(adr&0x1F, value);
                break;
        }
    }
    else if(adr >= 0x4020) // cartridge
        emu.crWrite(adr, value);
    else
        assert(FALSE);
}

static inline u8 cpu_get_status()
{
    return (cpu.carryFlag) | (cpu.zeroFlag<<1) | (cpu.intFlag<<2) | (cpu.decimalFlag<<3) | (cpu.overflowFlag<<6) | (cpu.signFlag<<7) | 0x20;
}

static inline u8 cpu_set_status(u8 status)
{
    cpu.carryFlag = status&0x1;
    cpu.zeroFlag = (status&0x2)>>1;
    cpu.intFlag = (status&0x4)>>2;
    cpu.decimalFlag = (status&0x8)>>3;
    cpu.overflowFlag = (status&0x40)>>6;
    cpu.signFlag = (status&0x80)>>7;
}

void cpu_nmi_trigger()
{
    u16 from = cpu.PC;
    PUSH_BYTE((cpu.PC>>8)&0xFF);
    PUSH_BYTE(cpu.PC&0xFF);
    PUSH_BYTE(cpu_get_status());
    cpu.intFlag = 1;
    u16 jmpLoc = (read_byte(0xFFFA) | (read_byte(0xFFFB) << 8));
    cpu.PC = jmpLoc;
}

void cpu_cycle()
{
    u8 op, instr, adrMode, src;
    u16 adr;

    if(cpu.suspClocks > 0) // if clocks left from previous instruction then do nothing
        goto cpu_end_cycle;

    if((cpu.strobe & 1) != 0) // if controller strobing enabled then strobe them
    {
        cpu.pad1Data = buttons;
        cpu.pad2Data = 0;
    }

    op = FETCH_PC_BYTE();
    instr = decode_table[op];
    adrMode = adr_mode_table[op];
    cpu.suspClocks = clock_table[op];

#define GET_SRC_VAL() ( adrMode == ADR_MODE_IMMEDIATE || adrMode == ADR_MODE_IMPLIED ? src : read_byte(adr))

    //printf("Execute %s(0x%02x) %s clk: %d\n", inst_to_string(instr), op, adr_mode_to_string(adrMode), clk);

    switch(adrMode)
    {
        case ADR_MODE_IMPLIED:
            src = cpu.A;
            read_byte(cpu.PC); // dummy read for emulation
            break;
        case ADR_MODE_ZP:
            {
                adr = FETCH_PC_BYTE();
            } break;
        case ADR_MODE_IMMEDIATE:
            src = FETCH_PC_BYTE();
            break;
        case ADR_MODE_ABS:
            {
                u8 adrlow = FETCH_PC_BYTE();
                u8 adrhigh = FETCH_PC_BYTE();
                adr = (adrhigh << 8) | adrlow;
            } break;
        case ADR_MODE_ZP_INDEXED_X:
            {
                adr = FETCH_PC_BYTE();
                adr = ((adr+cpu.X)&0xFF);
            } break;
        case ADR_MODE_ZP_INDEXED_Y:
            adr = FETCH_PC_BYTE();
            adr = ((adr+cpu.Y)&0xFF);
            break;
        case ADR_MODE_ABS_INDEXED_X:
            {
                u8 adrlow = FETCH_PC_BYTE();
                u8 adrhigh = FETCH_PC_BYTE();
                adr = (adrhigh << 8) | adrlow;
                adr += cpu.X;
            } break;
        case ADR_MODE_ABS_INDEXED_Y:
            {
                u8 adrlow = FETCH_PC_BYTE();
                u8 adrhigh = FETCH_PC_BYTE();
                adr = (adrhigh << 8) | adrlow;
                adr += cpu.Y;
            } break;
        case ADR_MODE_PC_RELATIVE:
            {
                u8 rel = FETCH_PC_BYTE();
                adr = cpu.PC;
                if(rel & 0x80)
                    adr += (rel-0x100);
                else
                    adr += rel;
            } break;
        case ADR_MODE_INDEXED_INDIR:
            {
                u8 adrl = FETCH_PC_BYTE();
                u8 f = read_byte((adrl+cpu.X)&0xFF);
                u8 s = read_byte((adrl+cpu.X+1)&0xFF);
                adr = f+(s<<8);
            } break;
        case ADR_MODE_INDIR_INDEXED:
            {
                u8 adrl = FETCH_PC_BYTE();
                u8 f = read_byte(adrl);
                u8 s = read_byte((adrl+1)&0xFF);
                adr = f+(s<<8)+cpu.Y;
            } break;
        case ADR_MODE_JMP_INDIR:
            {
                u8 adrlow = FETCH_PC_BYTE();
                u8 adrhigh = FETCH_PC_BYTE();
                adr = (adrhigh << 8) | adrlow;
                adrlow = read_byte(adr);
                adrhigh = read_byte(adr+1);
                adr = (adrhigh << 8) | adrlow;
            } break;
        default:
            assert(FALSE); // unknown addressing mode
            break;
    }

    switch(instr)
    {
        case OP_SEI:
            cpu.intFlag = 1;
            break;
        case OP_CLI:
            cpu.intFlag = 0;
            break;
        case OP_SED:
            cpu.decimalFlag = 1;
            break;
        case OP_CLD:
            cpu.decimalFlag = 0;
            break;
        case OP_CLV:
            cpu.overflowFlag = 0;
            break;
        case OP_SEC:
            cpu.carryFlag = 1;
            break;
        case OP_CLC:
            cpu.carryFlag = 0;
            break;
        case OP_INX:
            cpu.X++;
            SET_ZERO_FLAG_VAL(cpu.X);
            SET_SIGN_FLAG_VAL(cpu.X);
            break;
        case OP_INY:
            cpu.Y++;
            SET_ZERO_FLAG_VAL(cpu.Y);
            SET_SIGN_FLAG_VAL(cpu.Y);
            break;
        case OP_DEC:
            src = GET_SRC_VAL();
            src -= 1;
            SET_ZERO_FLAG_VAL(src);
            SET_SIGN_FLAG_VAL(src);
            write_byte(adr, src);
            break;
        case OP_LDA:
            src = GET_SRC_VAL();
            cpu.A = src;
            SET_ZERO_FLAG_VAL(src);
            SET_SIGN_FLAG_VAL(src);
            break;
        case OP_BPL:
            if (!cpu.signFlag) {
                cpu.PC = adr;
            } break;
        case OP_BMI:
            if (cpu.signFlag) {
                cpu.PC = adr;
            } break;

        case OP_BCS:
            if (cpu.carryFlag) {
                cpu.PC = adr;
            } break;
        case OP_BCC:
            if (!cpu.carryFlag) {
                cpu.PC = adr;
            } break;

        case OP_BNE:
            if (!cpu.zeroFlag) {
                cpu.PC = adr;
            } break;
        case OP_BEQ:
            if(cpu.zeroFlag) {
                cpu.PC = adr;
            } break;
        case OP_BVS:
            if(cpu.overflowFlag) {
                cpu.PC = adr;
            } break;
        case OP_BVC:
            if(!cpu.overflowFlag) {
                cpu.PC = adr;
            } break;
        case OP_JMP:
            cpu.PC = adr;
            break;
        case OP_BRK:
            {
                cpu.PC++;
                PUSH_BYTE((cpu.PC>>8)&0xFF);
                PUSH_BYTE(cpu.PC&0xFF);
                PUSH_BYTE(cpu_get_status() | 0x10);
                cpu.intFlag = (1);
                u16 jmpLoc = (read_byte(0xFFFE) | (read_byte(0xFFFF) << 8));
                cpu.PC = jmpLoc;
            } break;
        case OP_RTI:
            {
                u8 val = POP_BYTE();
                cpu_set_status(val);
                u8 retLow = POP_BYTE();
                u8 retHigh = POP_BYTE();
                u16 retAdr = (retHigh << 8) | retLow;
                cpu.PC = retAdr;
            } break;
        case OP_ORA:
            src = GET_SRC_VAL();
            src |= cpu.A;
            SET_ZERO_FLAG_VAL(src);
            SET_SIGN_FLAG_VAL(src);
            cpu.A = src;
            break;
        case OP_NOP:
            break;
        case OP_ASL:
            if(adrMode == ADR_MODE_IMPLIED)
            {
                cpu.carryFlag = ((cpu.A & 0x80) != 0);
                cpu.A <<= 1;
                SET_SIGN_FLAG_VAL(cpu.A);
                SET_ZERO_FLAG_VAL(cpu.A);
            }
            else
            {
                src = GET_SRC_VAL();
                cpu.carryFlag = ((src & 0x80) != 0);
                u8 val = src << 1;
                write_byte(adr, val);
                SET_SIGN_FLAG_VAL(val);
                SET_ZERO_FLAG_VAL(val);
            }
            break;
        case OP_LSR:
            {
                if(adrMode == ADR_MODE_IMPLIED)
                {
                    cpu.carryFlag = ((cpu.A & 0x01) != 0);
                    cpu.A >>= 1;
                    SET_SIGN_FLAG_VAL(cpu.A);
                    SET_ZERO_FLAG_VAL(cpu.A);
                }
                else
                {
                    src = GET_SRC_VAL();
                    cpu.carryFlag = ((src & 0x01) != 0);
                    u8 val = src >> 1;
                    write_byte(adr, val);
                    SET_SIGN_FLAG_VAL(val);
                    SET_ZERO_FLAG_VAL(val);
                }

            } break;
       case OP_STA:
            write_byte(adr, cpu.A);
            break;
        case OP_STX:
            write_byte(adr, cpu.X);
            break;
        case OP_STY:
            write_byte(adr, cpu.Y);
            break;
        case OP_LDX:
            src = GET_SRC_VAL();
            cpu.X = src;
            SET_SIGN_FLAG_VAL(src);
            SET_ZERO_FLAG_VAL(src);
            break;
        case OP_LDY:
            src = GET_SRC_VAL();
            cpu.Y = src;
            SET_SIGN_FLAG_VAL(src);
            SET_ZERO_FLAG_VAL(src);
            break;
        case OP_CMP:
            {
                src = GET_SRC_VAL();
                u16 val = cpu.A - src;
                cpu.carryFlag = (val < 0x100);
                SET_SIGN_FLAG_VAL(val&0xFF);
                SET_ZERO_FLAG_VAL(val&0xFF);
            } break;
        case OP_CPX:
            {
                src = GET_SRC_VAL();
                u16 val = cpu.X - src;
                cpu.carryFlag = (val < 0x100);
                SET_SIGN_FLAG_VAL(val&0xFF);
                SET_ZERO_FLAG_VAL(val&0xFF);
            } break;
        case OP_CPY:
            {
                src = GET_SRC_VAL();
                u16 val = cpu.Y - src;
                cpu.carryFlag = (val < 0x100);
                SET_SIGN_FLAG_VAL(val&0xFF);
                SET_ZERO_FLAG_VAL(val&0xFF);
            } break;
        case OP_TXS:
            cpu.S = cpu.X;
            break;
        case OP_JSR:
            cpu.PC--;
            PUSH_BYTE((cpu.PC>>8)&0xFF);
            PUSH_BYTE(cpu.PC&0xFF);
            cpu.PC = adr;
            break;
        case OP_RTS:
            adr = POP_BYTE()&0xFF;
            adr += (POP_BYTE() << 8) + 1;
            cpu.PC = adr;
            break;
        case OP_DEY:
            src = cpu.Y - 1;
            SET_SIGN_FLAG_VAL(src);
            SET_ZERO_FLAG_VAL(src);
            cpu.Y = src;
            break;
        case OP_DEX:
            src = cpu.X - 1;
            SET_SIGN_FLAG_VAL(src);
            SET_ZERO_FLAG_VAL(src);
            cpu.X = src;
            break;
        case OP_BIT:
            src = GET_SRC_VAL();
            SET_SIGN_FLAG_VAL(src);
            cpu.overflowFlag = ((0x40 & src) != 0);
            SET_ZERO_FLAG_VAL(src & cpu.A);
            break;
        case OP_AND:
            src = GET_SRC_VAL();
            src &= cpu.A;
            SET_SIGN_FLAG_VAL(src);
            SET_ZERO_FLAG_VAL(src);
            cpu.A = src;
            break;
        case OP_EOR:
            src = GET_SRC_VAL();
            src ^= cpu.A;
            SET_SIGN_FLAG_VAL(src);
            SET_ZERO_FLAG_VAL(src);
            cpu.A = src;
            break;
        case OP_TXA:
            SET_SIGN_FLAG_VAL(cpu.X);
            SET_ZERO_FLAG_VAL(cpu.X);
            cpu.A = cpu.X;
            break;
        case OP_TYA:
            SET_SIGN_FLAG_VAL(cpu.Y);
            SET_ZERO_FLAG_VAL(cpu.Y);
            cpu.A = cpu.Y;
            break;
        case OP_TAX:
            SET_SIGN_FLAG_VAL(cpu.A);
            SET_ZERO_FLAG_VAL(cpu.A);
            cpu.X = cpu.A;
            break;
        case OP_TAY:
            SET_SIGN_FLAG_VAL(cpu.A);
            SET_ZERO_FLAG_VAL(cpu.A);
            cpu.Y = cpu.A;
            break;
        case OP_TSX:
            src = cpu.S;
            SET_SIGN_FLAG_VAL(src);
            SET_ZERO_FLAG_VAL(src);
            cpu.X = src;
            break;
        case OP_PHA:
            PUSH_BYTE(cpu.A);
            break;
        case OP_PLA:
            src = POP_BYTE();
            SET_SIGN_FLAG_VAL(src);
            SET_ZERO_FLAG_VAL(src);
            cpu.A = src;
            break;
        case OP_INC:
            src = GET_SRC_VAL();
            src += 1;
            SET_SIGN_FLAG_VAL(src);
            SET_ZERO_FLAG_VAL(src);
            write_byte(adr, src);
            break;
        case OP_ROL:
            {
                u16 val = GET_SRC_VAL();
                val <<= 1;
                if(cpu.carryFlag)
                    val |= 0x01;
                cpu.carryFlag = (val > 0xff);
                SET_SIGN_FLAG_VAL(val&0xFF);
                SET_ZERO_FLAG_VAL(val&0xFF);
                if(adrMode == ADR_MODE_IMPLIED)
                    cpu.A = val&0xFF;
                else
                    write_byte(adr, val&0xFF);
            } break;
        case OP_ROR:
            {
                u16 val = GET_SRC_VAL();
                if(cpu.carryFlag)
                    val |= 0x100;
                cpu.carryFlag = (val & 0x01);
                val >>= 1;
                SET_SIGN_FLAG_VAL(val & 0xFF);
                SET_ZERO_FLAG_VAL(val & 0xFF);
                if(adrMode == ADR_MODE_IMPLIED)
                    cpu.A = val&0xFF;
                else
                    write_byte(adr, val&0xFF);
            } break;

        case OP_SBC:
            {
                src = GET_SRC_VAL();
                u16 val = cpu.A - src - (cpu.carryFlag ? 0 : 1);
                SET_SIGN_FLAG_VAL(val & 0xFF);
                SET_ZERO_FLAG_VAL(val & 0xFF);
                cpu.overflowFlag = ((((cpu.A ^ val) & 0x80) && ((cpu.A ^ src) & 0x80)));
                cpu.carryFlag = (val < 0x100);
                cpu.A = (val & 0xFF);

            } break;
        case OP_ADC:
            {
                src = GET_SRC_VAL();
                u16 val = src + cpu.A + (cpu.carryFlag ? 1 : 0);
                SET_ZERO_FLAG_VAL(val & 0xFF);
                SET_SIGN_FLAG_VAL(val&0xFF);
                cpu.overflowFlag = (!((cpu.A ^ src) & 0x80) && ((cpu.A ^ val) & 0x80));
                cpu.carryFlag = (val > 0xFF);
                cpu.A = val&0xFF;
            } break;
        case OP_PHP:
            PUSH_BYTE(cpu_get_status());
            break;
        case OP_PLP:
            src = POP_BYTE();
            cpu_set_status(src);
            break;
        default:
            fprintf(stderr, "Execute %s(0x%02x) %s clk: %d\n", inst_to_string(instr), op, adr_mode_to_string(adrMode), clk);
            fprintf(stderr, "Unknown instruction \n");
            assert(FALSE);
            break;
    }
    assert(cpu.suspClocks != 0);
cpu_end_cycle:
    cpu.suspClocks--;
    clk++;
    //printf("A: %02x X: %02x, Y: %02x, P: %02x adr:%04x src:%04x\n", (u32)cpu.A, (u32)cpu.X, (u32)cpu.Y, (u32)cpu.P, adr, src);
}

