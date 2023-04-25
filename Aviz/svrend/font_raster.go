package svrend

// #cgo pkg-config: freetype2
/*
#include <ft2build.h>
#include FT_FREETYPE_H
#include <stdio.h>
*/
import "C"

//import "image/png"
import "image"
import "unsafe"
import "image/color"
import "image/png"
import "os"
import "bufio"

import gl "github.com/go-gl/gl/v3.3-core/gl"

const (
    fontAtlasSize = 1024
    initialTextVboSize = 1024 // num of quads
)

type GlyphKey struct {
    rune rune
    size uint32
}

type CachedGlyph struct {
    uvRect V4 // UV coords x, y, w, h
    rune rune
    size uint32
    // TODO: texture id etc

    // metrics
    width float32;
    height float32;
    bearingX float32;
    bearingY float32;
    advance float32;
}

type TextVertex struct {
    pos V3
    color V4
    texCoord V2
}

type FontRenderState struct {
    Dpi uint32
    ftLibrary C.FT_Library
    ftFace C.FT_Face
    fontCacheImage *image.Alpha
    cachedGlyphs map[GlyphKey]CachedGlyph
    packState RectPackState

    cacheDirty bool

    vao, vbo, ebo uint32
    vboSize int
    eboSize int
    glTexture uint32
    textVertices []TextVertex
    numTextVertices int
}

func InitFontRenderState(frs *FontRenderState, fontName string, dpi uint32) {
    err := C.FT_Init_FreeType(&frs.ftLibrary)
    if(err < 0) {
        panic("Init FreeType2 failed!")
    }
    err = C.FT_New_Face(frs.ftLibrary, C.CString(fontName), 0, &frs.ftFace)
    if(err != 0) {
        println("FT_New_Face failed! The font" + fontName + " probably isn't present!")
    }
    // TODO: this aint right
    err = C.FT_Set_Char_Size(frs.ftFace, 0, 32<<6, 96, 96)
    if(err != 0) {
        println("FT_Set_Char_size failed!")
    }
    frs.fontCacheImage = image.NewAlpha(image.Rect(0, 0, fontAtlasSize, fontAtlasSize))
    frs.cachedGlyphs = make(map[GlyphKey]CachedGlyph)
    InitRectPackState(&frs.packState, fontAtlasSize, fontAtlasSize)

    var glTexture uint32
    gl.GenTextures(1, &glTexture)
    gl.BindTexture(gl.TEXTURE_2D, glTexture)
    gl.TexImage2D(gl.TEXTURE_2D, 0, gl.RED, fontAtlasSize, fontAtlasSize, 0, gl.RED, gl.UNSIGNED_BYTE, nil)
    gl.TexParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.NEAREST)
    gl.TexParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.NEAREST)
    gl.TexParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_S, gl.CLAMP_TO_BORDER)
    gl.TexParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_T, gl.CLAMP_TO_BORDER)
    frs.glTexture = glTexture

    var vao, vbo, ebo uint32
    gl.GenVertexArrays(1, &vao)
    gl.BindVertexArray(vao)
    gl.GenBuffers(1, &vbo)
    gl.BindBuffer(gl.ARRAY_BUFFER, vbo)
    frs.vboSize = initialTextVboSize*4*4;
    gl.BufferData(gl.ARRAY_BUFFER, frs.vboSize, nil, gl.DYNAMIC_DRAW)
    gl.GenBuffers(1, &ebo)
    gl.BindBuffer(gl.ELEMENT_ARRAY_BUFFER, ebo)
    frs.textVertices = make([]TextVertex, initialTextVboSize)
    indices := make([]uint16, initialTextVboSize*6)
    for i := 0; i < initialTextVboSize; i++ {
        indices[i*6 + 0] = uint16(i*4+0);
        indices[i*6 + 1] = uint16(i*4+1);
        indices[i*6 + 2] = uint16(i*4+2);
        indices[i*6 + 3] = uint16(i*4+1);
        indices[i*6 + 4] = uint16(i*4+3);
        indices[i*6 + 5] = uint16(i*4+2);
        if (i+1)*6 >= 65536 {
            panic("16 bit text index buffer overflow!")
        }
    }
    frs.eboSize = initialTextVboSize*2*6;
    gl.BufferData(gl.ELEMENT_ARRAY_BUFFER, frs.eboSize, unsafe.Pointer(&indices[0]), gl.STATIC_DRAW)
    gl.EnableVertexAttribArray(0) // pos
    gl.VertexAttribPointer(0, 3, gl.FLOAT, false, 36, nil)
    gl.EnableVertexAttribArray(1)
    gl.VertexAttribPointer(1, 4, gl.FLOAT, false, 36, gl.PtrOffset(3*4))
    gl.EnableVertexAttribArray(2) // texcoord
    gl.VertexAttribPointer(2, 2, gl.FLOAT, false, 36, gl.PtrOffset(3*4 + 4*4))
    frs.vao = vao
    frs.vbo = vbo
    frs.ebo = ebo
    gl.BindVertexArray(0)

    frs.Dpi = dpi
}

func DestroyFontRender(frs *FontRenderState) {
    C.FT_Done_FreeType(frs.ftLibrary)
    C.FT_Done_Face(frs.ftFace)
    frs.fontCacheImage = nil

    gl.DeleteBuffers(1, &frs.vbo)
    gl.DeleteBuffers(1, &frs.ebo)
    gl.DeleteVertexArrays(1, &frs.vao)
}

func (f *FontRenderState) CacheGlyph(rune rune, size uint32) CachedGlyph {
    err := C.FT_Set_Char_Size(f.ftFace, 0, C.long(size<<6), C.uint(f.Dpi), C.uint(f.Dpi))
    if err != 0 {
        panic("FT_Set_Char_size failed!")
    }
    glyphIndex := C.FT_Get_Char_Index(f.ftFace, C.ulong(rune))
    // TODO: error handling?
    err = C.FT_Load_Glyph(f.ftFace, glyphIndex, C.FT_LOAD_DEFAULT)
    if err != 0 {
        panic("FT_Load_Glyph failed!")
    }
    err = C.FT_Render_Glyph(f.ftFace.glyph, C.FT_RENDER_MODE_NORMAL)
    if err != 0 {
        panic("FT_Render_Glyph failed!")
    }
    m := &f.ftFace.glyph.metrics
    glyphSize := IV2{int32(f.ftFace.glyph.bitmap.width), int32(f.ftFace.glyph.bitmap.rows)}
    atlasPos := f.packState.PlaceRectangle(glyphSize)
    cglyph := CachedGlyph {
        uvRect: V4 {
            float32(atlasPos.X)/float32(fontAtlasSize),
            float32(atlasPos.Y)/float32(fontAtlasSize),
            float32(glyphSize.X)/float32(fontAtlasSize),
            float32(glyphSize.Y)/float32(fontAtlasSize),
        }, rune:rune, size:size,
        width: float32(m.width>>6), // TODO: fractional part?
        height: float32(m.height>>6),
        bearingX: float32(m.horiBearingX>>6),
        bearingY: float32(m.horiBearingY>>6),
        advance: float32(m.horiAdvance>>6),
    }
    data := C.GoBytes(unsafe.Pointer(f.ftFace.glyph.bitmap.buffer), C.int(glyphSize.X*glyphSize.Y))
    //print("Cache at ", string(rune), " ", atlasPos.X, " ", atlasPos.Y, " ", glyphSize.X, " ", glyphSize.Y, " \n")
    for i := 0; i < int(glyphSize.X); i++ {
        for j := 0; j < int(glyphSize.Y); j++ {
            // TODO: there has to be a better way?
            f.fontCacheImage.Set(i+int(atlasPos.X), j+int(atlasPos.Y), color.Alpha{data[j*int(glyphSize.X)+i]})
        }
    }
    f.cachedGlyphs[GlyphKey{rune: rune, size:size}] = cglyph
    f.cacheDirty = true
    return cglyph
}

func (f *FontRenderState) GetGlyph(rune rune, size uint32) CachedGlyph {
    val, exists := f.cachedGlyphs[GlyphKey{rune: rune, size: size}]
    if !exists {
        val = f.CacheGlyph(rune, size)
    }
    return val
}

func (f *FontRenderState) debugOutputPng() {
    outFile, error := os.Create("font-atlas-debug-outptut.png")
    if error != nil {
        panic(error)
    }
    defer outFile.Close()
    b := bufio.NewWriter(outFile)
    error = png.Encode(b, f.fontCacheImage)
    if error != nil {
        panic(error)
    }
    error = b.Flush()
    if error != nil {
        panic(error)
    }
}

func (f *FontRenderState) addString(pos V3, str string, size uint32) {
    x := float32(0.0)
    ftCharL := C.FT_UInt(0)
    for _, char := range str {
        glyph := f.GetGlyph(char, size);
        w := glyph.width
        h := glyph.height
        bx := glyph.bearingX
        by := glyph.bearingY

        // TODO: try to cache any of this?
        ftCharR := C.FT_Get_Char_Index(f.ftFace, C.ulong(char))
        var kernFt C.FT_Vector
        err := C.FT_Get_Kerning(f.ftFace, ftCharL, ftCharR, C.FT_KERNING_DEFAULT, &kernFt)
        var kernX, kernY float32
        if err == 0 {
            kernX = float32(kernFt.x>>6)
            kernY = float32(kernFt.y>>6)
        } else {
            kernX = 0
            kernY = 0
        }
        ftCharL = ftCharR
        //println(kernX, " ", kernY)

        xBase := x+bx+kernX;
        yBase := by+kernY

        f.textVertices = append(f.textVertices, TextVertex {
            pos: V3Add(V3{xBase, -h+yBase, 0.0}, pos),
            color: V4{0.0, 0.0, 0.0, 1.0},
            texCoord: V2{glyph.uvRect.X, glyph.uvRect.Y+glyph.uvRect.W},
        })
        f.textVertices = append(f.textVertices, TextVertex {
            pos: V3Add(V3{xBase+w,-h+yBase, 0.0}, pos),
            color: V4{0.0, 0.0, 0.0, 1.0},
            texCoord: V2{glyph.uvRect.X+glyph.uvRect.Z, glyph.uvRect.Y+glyph.uvRect.W},
        })
        f.textVertices = append(f.textVertices, TextVertex {
            pos: V3Add(V3{xBase, yBase, 0.0}, pos),
            color: V4{0.0, 0.0, 0.0, 1.0},
            texCoord: V2{glyph.uvRect.X, glyph.uvRect.Y},
        })
        f.textVertices = append(f.textVertices, TextVertex {
            pos: V3Add(V3{xBase+w, yBase, 0.0}, pos),
            color: V4{0.0, 0.0, 0.0, 1.0},
            texCoord: V2{glyph.uvRect.X+glyph.uvRect.Z, glyph.uvRect.Y},
        })
        f.numTextVertices += 4
        x += glyph.advance+kernX
    }
}

func (f *FontRenderState) flushStrings(screenToClip *Mat4, program uint32) {
    gl.BindVertexArray(f.vao)
    nv := f.numTextVertices
    numTriangles := (nv/4)*6
    if nv*36 > f.vboSize {
        f.vboSize *= 2
        gl.BindBuffer(gl.ARRAY_BUFFER, f.vbo)
        gl.BufferData(gl.ARRAY_BUFFER, f.vboSize, nil, gl.DYNAMIC_DRAW)
    }
    if numTriangles*2 > f.eboSize {
        f.eboSize *= 2
        vcount := f.eboSize/12
        indices := make([]uint16, vcount*6)
        for i := 0; i < vcount; i++ {
            indices[i*6 + 0] = uint16(i*4+0);
            indices[i*6 + 1] = uint16(i*4+1);
            indices[i*6 + 2] = uint16(i*4+2);
            indices[i*6 + 3] = uint16(i*4+1);
            indices[i*6 + 4] = uint16(i*4+3);
            indices[i*6 + 5] = uint16(i*4+2);
            if (i+1)*6 >= 65536 {
                panic("16 bit text index buffer overflow!")
            }
        }
        gl.BindBuffer(gl.ELEMENT_ARRAY_BUFFER, f.ebo)
        gl.BufferData(gl.ELEMENT_ARRAY_BUFFER, f.eboSize, unsafe.Pointer(&indices[0]), gl.DYNAMIC_DRAW)
    }
    
    gl.BindBuffer(gl.ARRAY_BUFFER, f.vbo)
    gl.BufferSubData(gl.ARRAY_BUFFER, 0, nv*36, unsafe.Pointer(&f.textVertices[0].pos.X))

    gl.UseProgram(program)
    loc := gl.GetUniformLocation(program, gl.Str("_ModelToClip\x00"))
    gl.UniformMatrix4fv(loc, 1, false, &screenToClip.M[0][0])

    loc = gl.GetUniformLocation(program, gl.Str("_MainTex\x00"))
    gl.Uniform1i(loc, 0)
    gl.ActiveTexture(gl.TEXTURE0);
    gl.BindTexture(gl.TEXTURE_2D, f.glTexture)
    if(f.cacheDirty) {
        gl.TexSubImage2D(gl.TEXTURE_2D, 0, 0, 0, fontAtlasSize, fontAtlasSize, gl.RED, gl.UNSIGNED_BYTE, unsafe.Pointer(&f.fontCacheImage.Pix[0]))
        f.cacheDirty = false
    }

    gl.Disable(gl.DEPTH_TEST)
    gl.DrawElements(gl.TRIANGLES, int32(numTriangles), gl.UNSIGNED_SHORT, nil)
    f.textVertices = f.textVertices[:0]
    f.numTextVertices = 0
    gl.Enable(gl.DEPTH_TEST)
}
