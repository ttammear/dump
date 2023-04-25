package svrend

/*import "github.com/ttammear/freetype"
import "github.com/ttammear/freetype/truetype"
import "io/ioutil"
import "image"
//import "os"
//import "bufio"
//import "image/png"
import "golang.org/x/image/font"

import gl "github.com/go-gl/gl/v3.3-core/gl"

type CachedGlyph struct {
    uvRect V4;
    textureIndex uint32;
}

type SizedRune struct {
    rune rune
    size uint32
}

type FontCacheLine struct {
    // constant y offset in pixels
    yOffset uint32
    // x offset in pixels
    xOffset uint32
}

type FontCache struct {
    cachedGlyphs map[SizedRune]CachedGlyph
    // fontSize -> offset struct
    cacheLines map[uint32]FontCacheLine
    yOffset uint32
    glTexture uint32
    cacheImage *image.Alpha
    font *truetype.Font
    ftCtx *freetype.Context
}

func fontMain() {

    fontBytes, err := ioutil.ReadFile("font.ttf")
    if err != nil {
        panic(err)
    }
    f, err := freetype.ParseFont(fontBytes)
    if err != nil {
        panic(err)
    }

    rgba := image.NewAlpha(image.Rect(0, 0, 640, 480))

    c := freetype.NewContext()
    c.SetDPI(100)
    c.SetFont(f)
    c.SetFontSize(72)
    c.SetClip(rgba.Bounds())
    c.SetDst(rgba)
    c.SetSrc(image.Opaque)
    c.SetHinting(font.HintingFull)

    pt := freetype.Pt(10, 10+int(c.PointToFixed(72)>>6))
    _, err = c.DrawString("A", pt)
    if err != nil {
        panic(err)
    }
    pt.Y += c.PointToFixed(72)
    c.DrawString("B", pt)

    outFile, err := os.Create("out.png")
    defer outFile.Close()
    b := bufio.NewWriter(outFile)
    err = png.Encode(b, rgba)
    b.Flush()


    fc := new(FontCache)
    fc.cachedGlyphs = make(map[SizedRune]CachedGlyph)
    gl.GenTextures(1, &fc.glTexture)
    fc.cacheImage = rgba
    fc.font = f
    fc.ftCtx = c
}

func (fc *FontCache) GetGlyph(r rune, size uint32) CachedGlyph {
    sr := SizedRune{r, size}
    ret, found := fc.cachedGlyphs[sr]
    if !found {
        ret = fc.loadGlyph(r, size)
    }
    return ret
}

func (fc *FontCache) loadGlyph(r rune, size uint32) CachedGlyph {
    _, found := fc.cacheLines[size]
    if !found {
        // TODO: create new font cache line
        //lineStartY = fc->yOffset
    }
    index := fc.font.Index(r)
    pt := freetype.Pt(0, 0)
    fc.ftCtx.Glyph(index, pt)
    return CachedGlyph{}
}
*/
