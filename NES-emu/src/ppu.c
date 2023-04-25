#include "nes.h"
#include <assert.h>
#include <stdio.h>
#include <stdint.h>

u32 palette[64] =
{
    0xFF7C7C7C, 0xFFFC0000, 0xFFBC0000, 0xFFBC2844,
    0xFF840094, 0xFF2000A8, 0xFF0010A8, 0xFF001488,
    0xFF003050, 0xFF007800, 0xFF006800, 0xFF005800,
    0xFF584000, 0xFF000000, 0xFF000000, 0xFF000000,
    0xFFBCBCBC, 0xFFF87800, 0xFFF85800, 0xFFFC4468,
    0xFFCC00D8, 0xFF5800E4, 0xFF0038F8, 0xFF105CE4,
    0xFF007CAC, 0xFF00B800, 0xFF00A800, 0xFF44A800,
    0xFF888800, 0xFF000000, 0xFF000000, 0xFF000000,
    0xFFF8F8F8, 0xFFFCBC3C, 0xFFFC8868, 0xFFF87898,
    0xFFF878F8, 0xFF9858F8, 0xFF5878F8, 0xFF44A0FC,
    0xFF00B8F8, 0xFF18F8B8, 0xFF54D858, 0xFF98F858,
    0xFFD8E800, 0xFF787878, 0xFF000000, 0xFF000000,
    0xFFFCFCFC, 0xFFFCE4A4, 0xFFF8B8B8, 0xFFF8B8D8,
    0xFFF8B8F8, 0xFFC0A4F8, 0xFFB0D0F0, 0xFFA8E0FC,
    0xFF78D8F8, 0xFF78F8D8, 0xFFB8F8B8, 0xFFD8F8B8,
    0xFFFCFC00, 0xFFF8D8F8, 0xFF000000, 0xFF000000
};

/*u32 palette[64] =
{
    0x7C7C7CFF, 0x0000FCFF, 0x0000BCFF, 0x4428BCFF,
    0x940084FF, 0xA80020FF, 0xA81000FF, 0x881400FF,
    0x503000FF, 0x007800FF, 0x006800FF, 0x005800FF,
    0x004058FF, 0x000000FF, 0x000000FF, 0x000000FF,
    0xBCBCBCFF, 0x0078F8FF, 0x0058F8FF, 0x6844FCFF,
    0xD800CCFF, 0xE40058FF, 0xF83800FF, 0xE45C10FF,
    0xAC7C00FF, 0x00B800FF, 0x00A800FF, 0x00A844FF,
    0x008888FF, 0x000000FF, 0x000000FF, 0x000000FF,
    0xF8F8F8FF, 0x3CBCFCFF, 0x6888FCFF, 0x9878F8FF,
    0xF878F8FF, 0xF85898FF, 0xF87858FF, 0xFCA044FF,
    0xF8B800FF, 0xB8F818FF, 0x58D854FF, 0x58F898FF,
    0x00E8D8FF, 0x787878FF, 0x000000FF, 0x000000FF,
    0xFCFCFCFF, 0xA4E4FCFF, 0xB8B8F8FF, 0xD8B8F8FF,
    0xF8B8F8FF, 0xF8A4C0FF, 0xF0D0B0FF, 0xFCE0A8FF,
    0xF8D878FF, 0xD8F878FF, 0xB8F8B8FF, 0xB8F8D8FF,
    0x00FCFCFF, 0xF8D8F8FF, 0x000000FF, 0x000000FF
};*/

static const unsigned char BitReverseTable256[] = 
{
  0x00, 0x80, 0x40, 0xC0, 0x20, 0xA0, 0x60, 0xE0, 0x10, 0x90, 0x50, 0xD0, 0x30, 0xB0, 0x70, 0xF0, 
  0x08, 0x88, 0x48, 0xC8, 0x28, 0xA8, 0x68, 0xE8, 0x18, 0x98, 0x58, 0xD8, 0x38, 0xB8, 0x78, 0xF8, 
  0x04, 0x84, 0x44, 0xC4, 0x24, 0xA4, 0x64, 0xE4, 0x14, 0x94, 0x54, 0xD4, 0x34, 0xB4, 0x74, 0xF4, 
  0x0C, 0x8C, 0x4C, 0xCC, 0x2C, 0xAC, 0x6C, 0xEC, 0x1C, 0x9C, 0x5C, 0xDC, 0x3C, 0xBC, 0x7C, 0xFC, 
  0x02, 0x82, 0x42, 0xC2, 0x22, 0xA2, 0x62, 0xE2, 0x12, 0x92, 0x52, 0xD2, 0x32, 0xB2, 0x72, 0xF2, 
  0x0A, 0x8A, 0x4A, 0xCA, 0x2A, 0xAA, 0x6A, 0xEA, 0x1A, 0x9A, 0x5A, 0xDA, 0x3A, 0xBA, 0x7A, 0xFA,
  0x06, 0x86, 0x46, 0xC6, 0x26, 0xA6, 0x66, 0xE6, 0x16, 0x96, 0x56, 0xD6, 0x36, 0xB6, 0x76, 0xF6, 
  0x0E, 0x8E, 0x4E, 0xCE, 0x2E, 0xAE, 0x6E, 0xEE, 0x1E, 0x9E, 0x5E, 0xDE, 0x3E, 0xBE, 0x7E, 0xFE,
  0x01, 0x81, 0x41, 0xC1, 0x21, 0xA1, 0x61, 0xE1, 0x11, 0x91, 0x51, 0xD1, 0x31, 0xB1, 0x71, 0xF1,
  0x09, 0x89, 0x49, 0xC9, 0x29, 0xA9, 0x69, 0xE9, 0x19, 0x99, 0x59, 0xD9, 0x39, 0xB9, 0x79, 0xF9, 
  0x05, 0x85, 0x45, 0xC5, 0x25, 0xA5, 0x65, 0xE5, 0x15, 0x95, 0x55, 0xD5, 0x35, 0xB5, 0x75, 0xF5,
  0x0D, 0x8D, 0x4D, 0xCD, 0x2D, 0xAD, 0x6D, 0xED, 0x1D, 0x9D, 0x5D, 0xDD, 0x3D, 0xBD, 0x7D, 0xFD,
  0x03, 0x83, 0x43, 0xC3, 0x23, 0xA3, 0x63, 0xE3, 0x13, 0x93, 0x53, 0xD3, 0x33, 0xB3, 0x73, 0xF3, 
  0x0B, 0x8B, 0x4B, 0xCB, 0x2B, 0xAB, 0x6B, 0xEB, 0x1B, 0x9B, 0x5B, 0xDB, 0x3B, 0xBB, 0x7B, 0xFB,
  0x07, 0x87, 0x47, 0xC7, 0x27, 0xA7, 0x67, 0xE7, 0x17, 0x97, 0x57, 0xD7, 0x37, 0xB7, 0x77, 0xF7, 
  0x0F, 0x8F, 0x4F, 0xCF, 0x2F, 0xAF, 0x6F, 0xEF, 0x1F, 0x9F, 0x5F, 0xDF, 0x3F, 0xBF, 0x7F, 0xFF
};


u8 ppu_internal_ram[2048];
u8 ppu_spr_ram[256];
u8 ppu_secondary_oam[32];
u8 ppu_palette[32];

u8 ppu_get_register(u8 reg)
{
    u8 ret;
    assert(reg <= 7);
    switch(reg)
    { 
        case 2: // PPUSTATUS
            ret = ppu.STATUS;
            ppu.STATUS = ret & ~0x80;
            ppu.latchToggle = 0;
            break;
        case 4:
            // see description in read_oam_data()
            // unimplemented OAM bits are always 0
            if(ppu.scanline < 241)
                ret = ppu.OAM_DATA&0xE3;
            else
                ret = ppu_spr_ram[ppu.OAM_ADDR];
            break;
        case 7:
            ret = ppu_read_byte(ppu.accessAdr);
            if(ppu.accessAdr < 0x3F00)
            {
                u8 buf = ppu.bufferedData;
                ppu.bufferedData = ret;
                ret = buf;
            }
            else // the buffer will not be affected by palette ram, but read value will
                ppu.bufferedData = ppu_read_byte(ppu.accessAdr-0x1000);
            if(!(ppu.CTRL & 0x04))
                ppu.accessAdr += 1;
            else
                ppu.accessAdr += 32;
            break;
        default:
            ret = ppu.busLatch;
            break;
    }
    ppu.busLatch = ret;
    return ret;
}

void ppu_set_register(u8 reg, u8 value)
{
    //printf("ppu set register %d to %d\n", reg, value);
    assert(reg <= 7);
    switch(reg)
    {
        case 0:
            // TODO: remove hack (makes SMB work)
            // TODO: 8x16 sprites
            //if((ppu.MASK & 0x18) != 0)
                ppu.nameTblAdr = 0x2000 + (value & 0x3) * 0x400;
            // bit 3 = sprite table adr (8x8, ignored for 8x16) 0 = 0, 1=0x1000
            if((value & 0x08) != 0)
                ppu.spriteTblAdr = 0x1000;
            else
                ppu.spriteTblAdr = 0x0000;
            if((value & 0x10) != 0)
                ppu.bgTblAdr = 0x1000;
            else
                ppu.bgTblAdr = 0x0000;
            assert((value&0x40)==0);  
            ppu.CTRL = value;
            break;
        case 1:
            ppu.MASK = value;
            break;
        case 3: // OAM adr TODO: 2A03/7 support
            ppu.OAM_ADDR = value;    
            break;
        case 4: // OAM write
            // TODO: support weird unofficial behaviour
            if(ppu.scanline >= 241) // don't allow writes during rendering
            {
                ppu_spr_ram[ppu.OAM_ADDR] = value;
                ppu.OAM_ADDR++;
            }
            break;
        case 5:
            if(!ppu.latchToggle)
                ppu.scrollX = value;
            else
            {
                assert(value < 240); // TODO: not entirely correct
                ppu.scrollY = value;
            }
            ppu.latchToggle = !ppu.latchToggle;
            break;
        case 6:
        {
            if(!ppu.latchToggle)
                ppu.accessAdr = (value<<8)&0xFF00;
            else
                ppu.accessAdr |= value;
            ppu.latchToggle = !ppu.latchToggle;
        } break;
        case 7:
        {
            // incremend of ppu.accessAdr depens on bit 2 (3rd bit) of ppu CTRL
            // 0 - add 1 going across, 1 - add 32, going down
            u16 adrBefore = ppu.accessAdr;
            if(!(ppu.CTRL & 0x04))
                ppu.accessAdr += 1;
            else
                ppu.accessAdr += 32;
            ppu_write_byte(adrBefore, value);
        } break;
        default:
            *((u8*)&ppu + reg) = value;
            break;
    }
    ppu.busLatch = value;
}

u8 ppu_read_oam_data(u8 adr)
{
    u8 val = ppu_spr_ram[adr];
    // TODO: should also apply to scanline sprite RAM
    ppu.OAM_DATA = val; // reading OAM_DATA during render exposes internal OAM access
    return val;
}

static inline u8 ppu_read_nametable_byte(u16 adr)
{
    return emu.currentNtPtr[(adr&0xFFF)>>10][adr&0x3FF];
}

static inline void ppu_write_nametable_byte(u16 adr, u8 val)
{
    emu.currentNtPtr[(adr&0xFFF)>>10][adr&0x3FF] = val;
}

u8 ppu_read_byte(u16 adr)
{

    if(adr >= 0x0000 && adr < 0x1000)
    {
        return emu.currentChrLowPtr[adr];
    }
    else if(adr >= 0x1000 && adr < 0x2000)
    {
        return emu.currentChrHiPtr[adr-0x1000];
    }
    else if(adr >= 0x2000 && adr < 0x3000)
    {
        return ppu_read_nametable_byte(adr);
    }
    else if(adr >= 0x3000 && adr < 0x3F00) // mirror of 0x2000+
    {
        return ppu_read_nametable_byte(adr-0x1000);
    }
    else if(adr >= 0x3F00 && adr < 0x3FFF) // palette RAM
    {
        u32 index = adr&0x1F;
        switch(index)
        {
            case 0x10: index = 0x0; break;
            case 0x14: index = 0x4; break;
            case 0x18: index = 0x8; break;
            case 0x1C: index = 0xC; break;
            default: break;
        }
        return ppu_palette[index];
    }
    else
    {
        assert(FALSE); // ppu memory region not set
        return 0;
    }
}

void ppu_write_byte(u16 adr, u8 value)
{
    if(adr >= 0x0000 && adr < 0x1000) // TODO: should be read only?
    {
        emu.currentChrLowPtr[adr] = value;
        printf("ppu write RAM\n");
    }
    else if(adr >= 0x1000 && adr < 0x2000)
    {
        printf("ppu write RAM 2\n");
        emu.currentChrHiPtr[adr-0x1000] = value;
    }
    else if(adr >= 0x2000 && adr < 0x3000)
    {
        ppu_write_nametable_byte(adr, value);
    }
    else if(adr >= 0x3000 && adr < 0x3F00) // mirror of 0x2000+
    {
        ppu_write_nametable_byte(adr-0x1000, value);
    }
    else if(adr >= 0x3F00 && adr < 0x3FFF) // palette RAM
    {
        u32 index = adr&0x1F;
        switch(index)
        {
            case 0x10: index = 0x0; break;
            case 0x14: index = 0x4; break;
            case 0x18: index = 0x8; break;
            case 0x1C: index = 0xC; break;
            default: break;
        }
        ppu_palette[index] = value;
    }
}

static inline void ppu_eval_sprites(i32 sl)
{
    Scanline_Data *sld = &ppu.slData;
    u32 sc = 0;
    for(int i = 0; i < 64; i++)
    {
        // TODO: implement 8x16 sprites, also below when rendering
        //assert((ppu.CTRL & 0x20) == 0);
        u32 spriteY = ppu_read_oam_data(4*i);
        if(sl >= spriteY && sl < spriteY+8)
        {
            if(sc == 8) // set sprite overflow bit
            {
                ppu.STATUS |= 0x20;
                break;
            }
            assert(sc >= 0 && sc < 8);
            sld->sprites[sc].yPos = spriteY;
            sld->sprites[sc].idx = ppu_read_oam_data(4*i+1);
            sld->sprites[sc].attr = ppu_read_oam_data(4*i+2);
            sld->sprites[sc].xPos = ppu_read_oam_data(4*i+3);
            sld->sprites[sc].isSprite0 = (i == 0);
            sc++;
        }
    }
    sld->spriteCount = sc;
}

static inline void ppu_render_pixel(i32 sl, i32 col)
{
    Scanline_Data *sld = &ppu.slData;
    u32 bgColor = palette[ppu_read_byte(0x3F00)];
    u8 colB = 0;

    i32 x = col;
    i32 y = sl;

    u32 ocol = col;
    col += ppu.scrollX;
    u32 nt = (col >> 8) ? ppu.nameTblAdr + 0x400 : ppu.nameTblAdr;
    col &= 0xFF;

    u32 isLeftEdge = (col < 8);

    // background
    if((ppu.MASK & 0x08) != 0 && (!isLeftEdge || (ppu.MASK&0x2) != 0))
    {
        if((col & 0x7) == 0 || col == ppu.scrollX) // get nametable data
        {
            sld->indexBits = 0;
            u32 palX = col>>5; // div 32
            u32 palY = sl>>5; // div 32
            sld->palIdx = (palY<<3)+palX; // 8 per row
            u8 palB = ppu_read_byte(0x03C0 + nt + sld->palIdx); // read attribute table byte
            u32 pxM = col&0x1F; // modulus 32
            u32 slM = sl&0x1F;
            if(pxM<16 && slM<16) // top left
                sld->indexBits |= (palB<<2)&0xC;
            else if(pxM>=16 && slM<16) // top right
                sld->indexBits |= palB&0xC;
            else if(pxM<16 && slM>=16) // bottom left
                sld->indexBits |= (palB>>2)&0xC; 
            else if(pxM>=16 && slM>=16) // bottom right
                sld->indexBits |= (palB>>4)&0xC;
            u32 ntb = ppu_read_byte(nt + (col>>3) + ((sl&(~7))<<2)); // nt + col/8 + (scanline higher 5 bits)*4

            sld->lowSR = ppu_read_byte((ntb<<4)+(sl&0x7)+ppu.bgTblAdr);
            sld->highSR = ppu_read_byte((ntb<<4)+(sl&0x7)+ppu.bgTblAdr+8);
        }
        colB = ((sld->lowSR >> (7-(col&0x7))) & 1);
        colB |= ((sld->highSR >> (7-(col&0x7))) & 1) << 1;
        if((colB & 0x3) != 0)
            bgColor = palette[ppu_read_byte(0x3F00+(sld->indexBits|colB))];
    }
    
    // sprites
    if((ppu.MASK & 0x10) != 0 && (!isLeftEdge || (ppu.MASK&0x4) != 0))
    {

        Eval_Sprite *sprites = sld->sprites;
        for(int i = 0; i < sld->spriteCount; i++)
        {
            u16 spriteTblAdr;
            if((ppu.CTRL&0x20) == 0)
                spriteTblAdr = ppu.spriteTblAdr;
            else
                spriteTblAdr = (sprites[i].idx&1)==0?0x0000:0x1000;
            if(x >= sprites[i].xPos && x < sprites[i].xPos+8)
            {
                u32 yOfst = (y-1)-(sprites[i].yPos);
                u32 xOfst = x-sprites[i].xPos;
                assert(yOfst >= 0 && yOfst < 8); // y not on screen
                assert(xOfst >= 0 && xOfst < 8); // x not on screen
                u8 vFlip = yOfst;
                if((0x80 & sprites[i].attr) != 0)
                    vFlip = 7 - yOfst;
                u8 slowSR = ppu_read_byte((sprites[i].idx<<4)+vFlip+spriteTblAdr);
                u8 shighSR = ppu_read_byte((sprites[i].idx<<4)+vFlip+spriteTblAdr+8);
                u8 sft=7-xOfst;
                if((0x40 & sprites[i].attr) != 0) // horizontal flip
                    sft = xOfst;
                u8 scolB = ((slowSR >> sft)&1) | (((shighSR >>sft)&1)<<1);
                u8 palBits = (sprites[i].attr & 0x3)<<2;
                u8 colIndex = ppu_read_byte(0x3F10 + (palBits | scolB));
                // background not opaque
                if(colB == 0 && scolB != 0) 
                    bgColor = palette[colIndex];
                // front sprite, always above bg
                else if((sprites[i].attr & 0x20) == 0 && scolB != 0)
                    bgColor = palette[colIndex];
                // sprite0 hit
                if(scolB != 0 && colB != 0 && sprites[i].isSprite0 && col != 255)
                {
                    ppu.STATUS |= 0x40;
                }
            }
        }
    }
    assert(x >= 0 && x < 256);
    assert(y >= 0 && y < 240);
    displayBuffer[y][x] = bgColor;
}


// TODO: this should be set by ppu.CTRL&0x8
#define NES_SPRITE_CHR_TBL 0x0000
// TODO: this should be set by ppu.CTRL&0x10
#define NES_CHR_TBL 0x1000
#define NES_NAME_TBL 0x2000

void ppu_sprite_eval1(uint8_t scanline) {
    // cycles 1-64 clear secondary oam to 0xFF
    for(int i = 0; i < 32; i++) {
        ppu_secondary_oam[i] = 0xFF;
    }
    // linear search sprites on scanline
    int n = 0;
    for(int i = 0; i < 64;) {
        uint8_t y = ppu_spr_ram[i*4 + 0];
        ppu_secondary_oam[n*4 + 0] = y;
        if(scanline >= y && scanline < y+8) {
            ppu_secondary_oam[n*4 + 1] = ppu_spr_ram[i*4 + 1];
            ppu_secondary_oam[n*4 + 2] = ppu_spr_ram[i*4 + 2];
            ppu_secondary_oam[n*4 + 3] = ppu_spr_ram[i*4 + 3];
            n++;
        }
        i++;
        if(n == 8) {
            // real hardware would just disable writes to secondary oam
            break;
        }
    }
}

void ppu_sprite_eval2(uint8_t scanline, uint8_t counters[], uint8_t vals1[], uint8_t vals2[], uint8_t attributes[], uint8_t *spriteCount) {
    uint8_t y;
    uint8_t attr;
    for(int i = 0; i < 8; i++) {
        uint8_t attr = ppu_secondary_oam[i*4 + 2];
        counters[i] = ppu_secondary_oam[i*4 + 3]; // byte 3 is x pos
        attributes[i] = attr;
        if(attr&0x80) {// vflip
            y = 8 - (scanline - ppu_secondary_oam[i*4]);
        } else {
            y = scanline - ppu_secondary_oam[i*4];
        }
        // TODO: fetch patterns
        //
        uint16_t chrIdx = (ppu_secondary_oam[i*4 + 1]<<4); 
        vals1[i] = ppu_read_byte(NES_SPRITE_CHR_TBL + chrIdx + y);
        vals2[i] = ppu_read_byte(NES_SPRITE_CHR_TBL + chrIdx + y + 8);
        if(attr&0x40) { // hflip
            vals1[i] = BitReverseTable256[vals1[i]];
            vals2[i] = BitReverseTable256[vals2[i]];
        }    
    }
}

void ppu_frame()
{
    uint16_t pattern1;
    uint16_t pattern2;

    uint8_t palette1;

    int ofst = 0;

    // palette bytes are fetched  every 8th cycle
    // every scanline is 341 cycles
    // total 262 scanlanes -1 to 261
    // visible scanlines - 0-239
    // visible cycles - 1-256

    // nametables 0x2000, 0x2400, 0x2800, 0x2C00
    // pattern table 0x0000 or 0x1000

    // realoding pattern registers
    // get nametable byte (determines pattern for 8x8 area)
    // fetch higher and lower byte

    uint16_t ntRow = 0; // nametable tile 8x8

    uint32_t nameTblEntry = ppu_read_byte(NES_NAME_TBL + ntRow++);
    /*patternSel1 = ppu_read_byte(NES_CHR_TBL + (nameTblEntry<<4) + (0&7)); // scanline&7
    patternSel2 = ppu_read_byte(NES_CHR_TBL + (nameTblEntry<<4) + (0&7) + 8);*/
    pattern1 = (uint16_t)ppu_read_byte(NES_CHR_TBL + (nameTblEntry<<4))<<8;
    pattern2 = (uint16_t)ppu_read_byte(NES_CHR_TBL + (nameTblEntry<<4) + 8)<<8;
    //ntRow++;
    //nameTblEntry = ppu_read_byte(NES_NAME_TBL + ntRow);
    //patternB1 = ppu_read_byte(NES_CHR_TBL + (nameTblEntry<<4) + (0&7));
    //patternB2 = ppu_read_byte(NES_CHR_TBL + (nameTblEntry<<4) + (0&7) + 8);

    // x,y < 16 = top left mask 0x3
    // y < 16, x > 16 top right mask 0xC
    // y > 16, x < 16 bottom left mask 0x30
    // x,y > 16 bottom right mask 0xC0
    //palette1 = (ppu_read_byte(0x03C0 + NES_NAME_TBL + 8*(0/32) +(0/32))<<2)&0xC; // bit 2,3
    
    uint8_t spriteCounters[8];
    uint8_t spritePattern1[8];
    uint8_t spritePattern2[8];
    uint8_t spriteAttributes[8];

    for(int i = 0; i < 8; i++) {
        spriteCounters[i] = 0xFF;
    }

    u32 bgColor = 0;

    uint8_t bgOpaque = 0;
    uint8_t spriteBehind = 0;
    uint8_t spriteOpaque = 0;

    uint8_t bgCIdx = 0;

    uint8_t found = 0;
    int sp;

#define SHIFT_REG() \
    pattern1 <<=1; \
    pattern2 <<=1;

#define FETCH_COLOR() bgCIdx = ((pattern1>>15)&1)|((pattern2>>14)&2); \
    bgColor = palette[ppu_read_byte(0x3F00 + (palette1|bgCIdx))];

#define RENDER_PIXEL() *((u32*)displayBuffer+(ofst++)) = bgColor

#define RELOAD_REG() \
    nameTblEntry = ppu_read_byte(NES_NAME_TBL + ntRow++); \
    pattern1 |= ppu_read_byte(NES_CHR_TBL + (nameTblEntry<<4) + (j&7)); \
    pattern2 |= ppu_read_byte(NES_CHR_TBL + (nameTblEntry<<4) + (j&7) + 8) \

#define FETCH_SPRITE() \
            for(sp = 0, found = 0; sp < 8; sp++) { \
                if(spriteCounters[sp] != 0) { \
                    spriteCounters[sp]--; \
                } else if(!found) { /* sprite x counter is 0! */ \
                    uint8_t pattern; \
                    pattern = ((spritePattern1[sp]>>7)&1) | ((spritePattern2[sp]>>6)&2); \
                    spritePattern1[sp] <<= 1; \
                    spritePattern2[sp] <<= 1; \
                    if(pattern != 0) { /* opaque sprite */ \
                        spriteOpaque = 1; \
                        if(!(spriteAttributes[sp]&0x20) || bgCIdx == 0) { \
                            bgColor = palette[ppu_read_byte(0x3F10 + (((spriteAttributes[sp]<<2)&0xC) | pattern))]; \
                        } \
                        found = 1; \
                    } \
                } \
            } 


    for(int j = 0; j < 240; j++) {

        ppu_sprite_eval1(j+1);

        for(int i = 0; i < 16; i++) {

            uint8_t paletteTile = ((j>>3)&2) | (i&1); // 0 - top left, 1 - top right, 2 - bottom left, 3 - bottom right (in 32x32 area)
            uint8_t paletteByte = ppu_read_byte(0x03C0 + NES_NAME_TBL + ((j>>5)<<3) + (i>>1)); // it's an array paletteByte[8][8] for 32x32 tiles
            palette1 = ((paletteByte>>(paletteTile<<1))&0x3)<<2;

            RELOAD_REG();

            FETCH_COLOR();
            FETCH_SPRITE();
            RENDER_PIXEL();
            SHIFT_REG();
            FETCH_COLOR();
            FETCH_SPRITE();
            RENDER_PIXEL();
            SHIFT_REG();
            FETCH_COLOR();
            FETCH_SPRITE();
            RENDER_PIXEL();
            SHIFT_REG();
            FETCH_COLOR();
            FETCH_SPRITE();
            RENDER_PIXEL();
            SHIFT_REG();
            FETCH_COLOR();
            FETCH_SPRITE();
            RENDER_PIXEL();
            SHIFT_REG();
            FETCH_COLOR();
            FETCH_SPRITE();
            RENDER_PIXEL();
            SHIFT_REG();
            FETCH_COLOR();
            FETCH_SPRITE();
            RENDER_PIXEL();
            SHIFT_REG();
            FETCH_COLOR();
            FETCH_SPRITE();
            RENDER_PIXEL();
            SHIFT_REG();

            // same nametable entry for 8 rows (at last tile of each row, next tile of next row must be loaded into shift registers)
            if(i == 15 && (ntRow&0x1F) == 0) { 
                ntRow = (((j+1)>>3)<<5); // ((j+1)/8)*32 (32 8x8 nt tiles per row)
            }
            RELOAD_REG();

            FETCH_COLOR();
            FETCH_SPRITE();
            RENDER_PIXEL();
            SHIFT_REG();
            FETCH_COLOR();
            FETCH_SPRITE();
            RENDER_PIXEL();
            SHIFT_REG();
            FETCH_COLOR();
            FETCH_SPRITE();
            RENDER_PIXEL();
            SHIFT_REG();
            FETCH_COLOR();
            FETCH_SPRITE();
            RENDER_PIXEL();
            SHIFT_REG();
            FETCH_COLOR();
            FETCH_SPRITE();
            RENDER_PIXEL();
            SHIFT_REG();
            FETCH_COLOR();
            FETCH_SPRITE();
            RENDER_PIXEL();
            SHIFT_REG();
            FETCH_COLOR();
            FETCH_SPRITE();
            RENDER_PIXEL();
            SHIFT_REG();
            FETCH_COLOR();
            FETCH_SPRITE();
            RENDER_PIXEL();
            SHIFT_REG();
        }
        ppu_sprite_eval2(j+1, spriteCounters, spritePattern1, spritePattern2, spriteAttributes, 0);
    }
}


b32 ppu_cycle() 
{
    b32 ret = FALSE;
    u32 slClk = ppu.clk%341;
    if(ppu.scanline == -1 && slClk == 1)
    {
        ppu.STATUS &= ~0x80; // clear VBLANK bit
        ppu.STATUS &= ~0x40; // clear Sprite0Hit
    }

    /*if(ppu.scanline >= 0 && ppu.scanline < 240 && slClk > 0 && slClk < 257) {

        ppu_render_pixel(ppu.scanline, slClk-1);
    }
    else if(slClk == 257)
        ppu_eval_sprites(ppu.scanline); */

    if(ppu.scanline == 241 && slClk == 1) // vblank start
    {
        ppu.STATUS |= 0x80; // set VBLANK bit
        if((ppu.CTRL & 0x80) != 0) // trigger NMI if NMI enabled
            cpu_nmi_trigger();
    }

    if(slClk == 340) // last clock for this scanline
    {
        if(ppu.scanline == 261) // last scanline
        {
            ppu.scanline = -1;
            ret = TRUE;
        }
        else
            ppu.scanline++;
    }
    ppu.clk++;
    return ret;
}


