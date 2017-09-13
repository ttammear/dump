#include "nes.h"
#include <assert.h>
#include <stdio.h>

/*u32 palette[64] =
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
};*/

u32 palette[64] =
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
};


u8 ppu_internal_ram[2048];
u8 ppu_spr_ram[256];
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
            u32 palX = col>>5;
            u32 palY = sl>>5;
            sld->palIdx = (palY<<3)+palX;
            u8 palB = ppu_read_byte(0x03C0 + nt + sld->palIdx);
            u32 pxM = col&0x1F;
            u32 slM = sl&0x1F;
            if(pxM<16 && slM<16) // top left
                sld->indexBits |= (palB&0x3)<<2;
            else if(pxM>=16 && slM<16) // top right
                sld->indexBits |= ((palB>>2)&0x3)<<2;
            else if(pxM<16 && slM>=16) // bottom left
                sld->indexBits |= ((palB>>4)&0x3)<<2; 
            else if(pxM>=16 && slM>=16) // bottom right
                sld->indexBits |= ((palB>>6)&0x3)<<2;
            u32 ntb = ppu_read_byte(nt + (col>>3) + ((sl&(~7))<<2));

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


b32 ppu_cycle() 
{
    b32 ret = FALSE;
    u32 slClk = ppu.clk%341;
    if(ppu.scanline == -1 && slClk == 1)
    {
        ppu.STATUS &= ~0x80; // clear VBLANK bit
        ppu.STATUS &= ~0x40; // clear Sprite0Hit
    }

    if(ppu.scanline >= 0 && ppu.scanline < 240 && slClk > 0 && slClk < 257)
        ppu_render_pixel(ppu.scanline, slClk-1);
    else if(slClk == 257)
        ppu_eval_sprites(ppu.scanline);         

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


