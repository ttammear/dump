package svrend

import gl "github.com/go-gl/gl/v3.3-core/gl"
import "C"
import "strings"
import "unsafe"

var vertShader string = `#version 330 core 
layout(location = 0) in vec4 position;
layout(location = 1) in vec4 color;
out vec4 v_color;
uniform mat4 _ModelToClip;
void main() {
    gl_Position = _ModelToClip*position;
    v_color = color;
}`

var fragShader string = `#version 330 core
layout(location = 0) out vec4 outColor;
in vec4 v_color;
uniform vec4 _Color;
void main() {
    vec3 outColorRGB = _Color.rgb*v_color.rgb;
    float alpha = _Color.a*v_color.a;
    outColor = vec4(outColorRGB*alpha, alpha);
}`

var wireTriangleVert string = `#version 330 core 
layout(location = 0) in vec4 position;
layout(location = 1) in vec4 color;
layout(location = 2) in vec3 bary;
out vec4 v_color;
out vec3 v_bary;
uniform mat4 _ModelToClip;
void main() {
    gl_Position = _ModelToClip*position;
    v_color = color;
    v_bary = bary;
}`

var wireTriangleFrag string = `#version 330 core
layout(location = 0) out vec4 outColor;
in vec4 v_color;
in vec3 v_bary;
uniform vec4 _Color;
void main() {
    vec3 d = abs(dFdx(v_bary)) + abs(dFdy(v_bary));
    const float borderSize = 3.0;
    const float smoothStepSize = 1.5;
    vec3 a = smoothstep(d*vec3(borderSize-smoothStepSize), d*borderSize, v_bary);
    float ne = min(min(a.x, a.y), a.z);
    outColor = mix(vec4(0.0, 0.0, 0.0, 1.0), v_color, ne);
}`


var textVert string = `#version 330 core 
layout(location = 0) in vec4 position;
layout(location = 1) in vec4 color;
layout(location = 2) in vec2 texCoord;
out vec4 v_color;
out vec2 v_texCoord;
uniform mat4 _ModelToClip;
void main() {
    gl_Position = _ModelToClip*position;
    v_color = color;
    v_texCoord = texCoord;
}`

var textFrag string = `#version 330 core
layout(location = 0) out vec4 outColor;
in vec4 v_color;
in vec2 v_texCoord;
uniform sampler2D _MainTex;
void main() {
    vec2 texCoord = vec2(v_texCoord.x, v_texCoord.y);
    vec3 outColorRGB = v_color.rgb;
    float alpha = v_color.a*texture2D(_MainTex, texCoord).r;
    outColor = vec4(outColorRGB*alpha, alpha);
}`

var lineVert string = `#version 330 core 
layout(location = 0) in vec4 position;
layout(location = 1) in vec4 color;
out vec4 v_color;
out vec3 v_worldPos;
uniform mat4 _ModelToView;
void main() {
    gl_Position = _ModelToView*position;
    v_color = color;
}`

var lineFrag string = `#version 330 core
layout(location = 0) out vec4 outColor;
in vec4 g_color;
uniform vec4 _Color;
void main() {
    vec3 outColorRGB = _Color.rgb*g_color.rgb;
    float alpha = _Color.a*g_color.a;
    outColor = vec4(outColorRGB*alpha, alpha);
}`

var lineGeom string = `#version 330 core
layout (lines_adjacency) in;
layout (triangle_strip, max_vertices = 32) out;
in vec4 v_color[4];
out vec4 g_color;

uniform mat4 _ViewToClip;
uniform float _LineWidth;

void debugLine(vec3 start, vec3 end, vec4 color) {
    vec3 lhs = normalize(cross(end-start, vec3(0.0, 0.0, -1.0)))*0.003;
    gl_Position = _ViewToClip*vec4(start+lhs, 1.0);
    g_color = color;
    EmitVertex();
    gl_Position = _ViewToClip*vec4(start-lhs, 1.0);
    EmitVertex();
    gl_Position = _ViewToClip*vec4(end+lhs, 1.0);
    EmitVertex();
    gl_Position = _ViewToClip*vec4(end-lhs, 1.0);
    EmitVertex();
    EndPrimitive();
}

void main() {
    vec3 prev3 = gl_in[0].gl_Position.xyz;
    vec3 start3 = gl_in[1].gl_Position.xyz;
    vec3 end3 = gl_in[2].gl_Position.xyz;
    vec3 next3 = gl_in[3].gl_Position.xyz;

    vec4 prevS4 = (_ViewToClip*gl_in[0].gl_Position);
    vec4 startS4 =( _ViewToClip*gl_in[1].gl_Position);
    vec4 endS4 = (_ViewToClip*gl_in[2].gl_Position);
    vec4 nextS4 = (_ViewToClip*gl_in[3].gl_Position);

    vec2 prevS = (prevS4/prevS4.w).xy;
    vec2 startS =(startS4/startS4.w).xy;
    vec2 endS = (endS4/endS4.w).xy;
    vec2 nextS = (nextS4/nextS4.w).xy;

    /*vec2 prev = gl_in[0].gl_Position.xy;
    vec2 start = gl_in[1].gl_Position.xy;
    vec2 end = gl_in[2].gl_Position.xy;
    vec2 next = gl_in[3].gl_Position.xy;*/
    vec2 prev = prevS;
    vec2 start = startS;
    vec2 end = endS;
    vec2 next = nextS;

    vec2 lhs = vec2(-normalize(end-start).y, normalize(end-start).x);

    // is previous line segment a zero vector?
    bool colStart = length(start-prev) < 0.0001 || abs(dot(normalize(start-prev), normalize(end-start))) > 0.999999; // 0.0001 is arbitrary epsilon
    // is next line segment a zero vector?
    bool colEnd = length(end-next) < 0.0001 || abs(dot(normalize(end-start), normalize(end-next))) > 0.999999;
    // TODO: if the vectors are very close to linear use intersection of lhs (end-start) vectors

    vec2 a = normalize(start-prev);
    vec2 b = normalize(start-end);
    vec2 c = (a+b)*0.5;
    vec2 startLhs = normalize(c) * sign(dot(c, lhs));
    a = normalize(end-start);
    b = normalize(end-next);
    c = (a+b)*0.5;
    vec2 endLhs = normalize(c) * sign(dot(c, lhs));

    if(colStart)
        startLhs = lhs;
    if(colEnd)
        endLhs = lhs;

    float startScale = dot(startLhs, lhs);
    float endScale = dot(endLhs, lhs);

    //debugLine(start3, start3+2*vec3(lhs, 0.0), vec4(0.0, 1.0, 0.0, 1.0));
    //debugLine(start3, start3+vec3(startLhs, 0.0), vec4(1.0, 1.0, 0.0, 1.0));
    //debugLine(start3, start3+vec3(0.001, 0.0, 1.0), vec4(0.0, 0.0, 0.0, 1.0));
    //debugLine(end3, start3, vec4(1.0, 0.0, 0.0, 1.0));

    startLhs *= _LineWidth;
    endLhs *= _LineWidth;

    vec4 debugColor = vec4(startScale, 0.0, 0.0, 1.0);
    /*if(length(start-prev) < 0.0001 || length(end-next) < 0.0001 || length(start-end) < 0.0001)
        debugColor = vec4(0.0, 1.0, 0.0, 1.0);*/
    g_color = debugColor;

    gl_Position = _ViewToClip*vec4(start3.xy+startLhs/startScale, start3.z, 1.0);
    g_color = v_color[1];
    EmitVertex();
    gl_Position = _ViewToClip*vec4(start3.xy-startLhs/startScale, start3.z, 1.0);
    EmitVertex();
    gl_Position = _ViewToClip*vec4(end3.xy+endLhs/endScale, end3.z, 1.0);
    g_color = v_color[2];
    EmitVertex();
    gl_Position = _ViewToClip*vec4(end3.xy-endLhs/endScale, end3.z, 1.0);
    EmitVertex();
    EndPrimitive();
}`

var rectangleVert string = `#version 330 core
layout(location = 0) in vec4 position;
layout(location = 1) in vec4 color;
out vec4 v_color;
out vec3 v_modelPos;
uniform mat4 _ModelToClip;
void main() {
    gl_Position = _ModelToClip*position;
    v_modelPos = position.xyz;
    v_color = color;
}`
var rectangleFrag string = `#version 330 core
layout(location = 0) out vec4 outColor;
in vec4 v_color;
in vec3 v_modelPos;
uniform vec4 _Color;
uniform vec4 _Outline;
void main() {
    const float r = 0.5;
    const float smoothAmount = 3.0;
    vec2 dist = abs(v_modelPos.xy);
    vec2 xdxy = dFdx(v_modelPos.xy);
    vec2 ydxy = dFdy(v_modelPos.xy);
    float ldx = length(vec2(xdxy.x, ydxy.x));
    float ldy = length(vec2(xdxy.y, ydxy.y));
    float bx = 1.0 - smoothAmount*ldx;
    float by = 1.0 - smoothAmount*ldy;
    vec2 b = vec2(bx, by);
    vec2 a = smoothstep(vec2(r), r*b, dist);
    float alpha = min(a.x, a.y)*v_color.a*_Color.a;

    // outline
    float ow = _Outline.w*3;
    vec2 bcs = vec2(1-ow*ldx, 1-ow*ldy);
    vec2 blendColor = smoothstep(r*bcs, r*b*bcs, dist);
    float blendColorm = min(blendColor.x, blendColor.y);

    vec3 oc = mix(_Outline.rgb, (_Color*v_color).rgb, blendColorm);
    outColor = vec4(oc*alpha, alpha);
}`

var circleVert string = `#version 330 core 
layout(location = 0) in vec4 position;
layout(location = 1) in vec4 color;
out vec4 v_color;
out vec3 v_modelPos;
uniform mat4 _ModelToClip;
void main() {
    gl_Position = _ModelToClip*position;
    v_modelPos = position.xyz;
    v_color = color;
}`
var circleFrag string = `#version 330 core
layout(location = 0) out vec4 outColor;
in vec4 v_color;
in vec3 v_modelPos;
uniform vec4 _Color;
uniform vec4 _Outline;
void main() {
    vec2 dx = dFdy(v_modelPos.xy);
    // TODO: non uniform scaling doesn't work (need derivative on y axis)
    float blendPrec = 1.0 - 4.0*length(dx);
    const float radius = 0.5;
    vec3 colRGB;
    float alpha;
    float dist = length(v_modelPos); // distance from circle center to current frag
    if(_Outline.a == 0.0) {
        colRGB = _Color.rgb*v_color.rgb;
        alpha = smoothstep(radius, radius*blendPrec, dist)*_Color.a*v_color.a;
    } else {
        alpha = smoothstep(radius, radius*blendPrec, dist)*_Color.a*v_color.a;
        float bcs = 1.0 - length(dx)*_Outline.a*3.0;
        float borderBlend = smoothstep(max(radius*bcs,0.1), radius*bcs*blendPrec, dist);
        colRGB = mix(_Outline.rgb, _Color.rgb*v_color.rgb, borderBlend);
    }
    outColor = vec4(colRGB*alpha, alpha);
}`

type RenderState struct {
    cameras []*Camera
    renderTargets []*RenderTarget
    activeCam *Camera
    width, height int32
    circleProgram, rectangleProgram, defaultProgram, lineProgram, textProgram, wireTriangleProgram uint32
    rectVao uint32

    rectangles []*Rectangle
    circles []*Circle
    linestrips []*Linestrip
    triangleMeshes []*TriangleMesh

    fontState FontRenderState
    DefaultRenderTarget RenderTarget
}

type Camera struct {
    Fov, ZNear, ZFar, AspectRatio float32
    Flags uint32
    Pos V3
    Rot Quat
    ViewToClip, WorldToView, WorldToClip Mat4
    renderTarget *RenderTarget
}

type RenderTarget struct {
    Width, Height, multisample int32
    // msReadColorTex is to GetPixels() on multisampled rendertarget
    depthRb, colorTex, fbo, msReadFbo, msReadColorTex uint32
    screenM Mat4
    OnRender func(*RenderTarget)
}

type Circle struct {
    pos V3
    radius float32
    color uint32
    outline V4
}

type Rectangle struct {
    Pos V3
    Rot Quat
    Width, Height float32
    Color uint32
    Outline V4
}

type LineVertex struct {
    pos V3
    color V4
}

type TriangleVertex struct {
    pos V3
    color V4
    bary V3
}

type Triangle struct {
    Pos [3]V3
    Color [3]V4
}

type TriangleMesh struct {
    triangles []Triangle
    Pos V3
    Rot Quat

    vao, vbo uint32
}

type Linestrip struct {
    Vertices []V3
    Colors []V4
    Width float32
    Outline V4

    meshVertices []LineVertex
    meshIndices []uint16
    meshVao, meshVbo, meshEbo uint32
}

func debugCallback(source, gltype, id, severity uint32, length int32, message string, userParam unsafe.Pointer) {
    println(message)
}

func (ls *Linestrip) Update(){
    vs := ls.Vertices
    cs := ls.Colors
    if len(vs) != len(cs) {
        panic("Linestrip vertices and colors count doesn't match")
    }
    if len(vs) >= 65536-2 {
        panic("Linestrip has too many vertices, should use uint32 indices?")
    }
    lsvs := ls.meshVertices
    if(lsvs == nil || len(lsvs) < len(vs) || len(lsvs) > len(vs)*2) {
        lsvs = make([]LineVertex, len(vs))
    }
    for i := 0; i < len(vs); i++ {
        lsvs[i] = LineVertex{vs[i], cs[i]}
    }
    ls.meshVertices = lsvs

    indexCount := len(vs)+2;
    // NOTE: unlike vertex buffer we never shrink this one
    if(ls.meshIndices == nil || len(ls.meshIndices) < indexCount) {
        ls.meshIndices = make([]uint16, indexCount)
    }
    ls.meshIndices[0] = 0;
    ls.meshIndices[indexCount-1] = uint16(indexCount-3);
    for i := 0; i < indexCount-2; i++ {
        ls.meshIndices[i+1] = uint16(i);
    }

    if ls.meshVao == 0 {
        gl.GenVertexArrays(1, &ls.meshVao)
        gl.BindVertexArray(ls.meshVao)
        // create vertex buffer
        gl.GenBuffers(1, &ls.meshVbo)
        gl.BindBuffer(gl.ARRAY_BUFFER, ls.meshVbo)
        gl.EnableVertexAttribArray(0) // position
        gl.VertexAttribPointer(0, 3, gl.FLOAT, false, 7*4, nil)
        gl.EnableVertexAttribArray(1) // color
        gl.VertexAttribPointer(1, 4, gl.FLOAT, false, 7*4, gl.PtrOffset(3*4))
        // create index buffer (need it for GL_LINES_ADJACENCY)
        gl.GenBuffers(1, &ls.meshEbo)
        gl.BindBuffer(gl.ELEMENT_ARRAY_BUFFER, ls.meshEbo)
    }
    // @OPTIMIZE: use single vertex buffer for all line strips
    gl.BindVertexArray(ls.meshVao)
    gl.BindBuffer(gl.ARRAY_BUFFER, ls.meshVbo)
    gl.BufferData(gl.ARRAY_BUFFER,7*4*len(vs), unsafe.Pointer(&lsvs[0]), gl.DYNAMIC_DRAW)
    gl.BindBuffer(gl.ELEMENT_ARRAY_BUFFER, ls.meshEbo)
    gl.BufferData(gl.ELEMENT_ARRAY_BUFFER, 2*len(ls.meshIndices), unsafe.Pointer(&ls.meshIndices[0]), gl.DYNAMIC_DRAW)
}

func (ls *Linestrip) destroy() {
    if ls.meshVao != 0 {
        gl.BindVertexArray(ls.meshVao)
        gl.DeleteBuffers(1, &ls.meshVbo)
        gl.DeleteBuffers(1, &ls.meshEbo)
        gl.DeleteVertexArrays(1, &ls.meshEbo)
    }
}

func compileShader(v, f, g string) uint32{
    var status int32

    vs := gl.CreateShader(gl.VERTEX_SHADER)
    vptr, vfree := gl.Strs(v)
    strLen := int32(len(v))
    gl.ShaderSource(vs, 1, vptr, &strLen)
    vfree()
    gl.CompileShader(vs)
    gl.GetShaderiv(vs, gl.COMPILE_STATUS, &status)
    if status == gl.FALSE {
        var logLength int32
        gl.GetShaderiv(vs, gl.INFO_LOG_LENGTH, &logLength)
        log := strings.Repeat("\x00", int(logLength+1))
        gl.GetShaderInfoLog(vs, logLength, nil, gl.Str(log))
        panic("Failed to compile vertex shader! Log:\n" + log)
    }

    fs := gl.CreateShader(gl.FRAGMENT_SHADER)
    fptr, ffree := gl.Strs(f)
    strLen = int32(len(f))
    gl.ShaderSource(fs, 1, fptr, &strLen)
    ffree()
    gl.CompileShader(fs)
    gl.GetShaderiv(fs, gl.COMPILE_STATUS, &status)
    if status == gl.FALSE {
        var logLength int32
        gl.GetShaderiv(fs, gl.INFO_LOG_LENGTH, &logLength)
        log := strings.Repeat("\x00", int(logLength+1))
        gl.GetShaderInfoLog(fs, logLength, nil, gl.Str(log))
        panic("Failed to compile fragment shader! Log:\n" + log)
    }

    ret := gl.CreateProgram()
    gl.AttachShader(ret, vs)
    gl.AttachShader(ret, fs)

    // TODO: restructure this
    if g != "" {
        gs := gl.CreateShader(gl.GEOMETRY_SHADER)
        vptr, vfree := gl.Strs(g)
        strLen := int32(len(g))
        gl.ShaderSource(gs, 1, vptr, &strLen)
        vfree()
        gl.CompileShader(gs)
        gl.GetShaderiv(gs, gl.COMPILE_STATUS, &status)
        if status == gl.FALSE {
            println("false")
            var logLength int32
            gl.GetShaderiv(gs, gl.INFO_LOG_LENGTH, &logLength)
            log := strings.Repeat("\x00", int(logLength+1))
            gl.GetShaderInfoLog(gs, logLength, nil, gl.Str(log))
            panic("Failed to compile geometry shader! Log:\n" + log)
        }
        gl.AttachShader(ret, gs)
    }

    gl.LinkProgram(ret)
    gl.GetProgramiv(ret, gl.LINK_STATUS, &status)
    if status == gl.FALSE {
        var logLength int32
        gl.GetProgramiv(ret, gl.INFO_LOG_LENGTH, &logLength)
        log := strings.Repeat("\x00", int(logLength+1))
        gl.GetProgramInfoLog(ret, logLength, nil, gl.Str(log))
        panic("Failed to link shader program! Log: \n" + log)
    }
    return ret
}

func (r *RenderState) Blit(src *RenderTarget, dst *RenderTarget) {
    drawFboId, readFboId := int32(0), int32(0)
    gl.GetIntegerv(gl.DRAW_FRAMEBUFFER_BINDING, &drawFboId)
    gl.GetIntegerv(gl.READ_FRAMEBUFFER_BINDING, &readFboId)

    if src.multisample == 0 {
        gl.BindFramebuffer(gl.READ_FRAMEBUFFER, src.fbo)
    } else if src.msReadFbo != 0 {
        gl.BindFramebuffer(gl.READ_FRAMEBUFFER, src.msReadFbo)
    } else {
        // TODO: implement?
        println("Could not blit multisampled framebuffer - no intermediate non-multisampled texture!")
        return
    }
    gl.BindFramebuffer(gl.DRAW_FRAMEBUFFER, dst.fbo)
    gl.BlitFramebuffer(0, 0, src.Width, src.Height, 0, 0, dst.Width, dst.Height, gl.COLOR_BUFFER_BIT, gl.NEAREST)

    gl.BindFramebuffer(gl.DRAW_FRAMEBUFFER, uint32(drawFboId))
    gl.BindFramebuffer(gl.READ_FRAMEBUFFER, uint32(readFboId))
}

func (r *RenderState) CreateRenderTarget(width, height int32, multiSample int32) *RenderTarget {

    rt := new(RenderTarget)

    drawFboId := int32(0)
    readFboId := int32(0)
    gl.GetIntegerv(gl.DRAW_FRAMEBUFFER_BINDING, &drawFboId)
    gl.GetIntegerv(gl.READ_FRAMEBUFFER_BINDING, &readFboId)

    var colorTex, fbo, depthRb uint32
    gl.GenFramebuffers(1, &fbo)
    gl.BindFramebuffer(gl.FRAMEBUFFER, fbo)

    gl.GenTextures(1, &colorTex)
    if multiSample == 0 {
        gl.BindTexture(gl.TEXTURE_2D, colorTex)
        gl.TexParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_S, gl.REPEAT)
        gl.TexParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_T, gl.REPEAT)
        gl.TexParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.NEAREST)
        gl.TexParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.NEAREST)
        gl.TexImage2D(gl.TEXTURE_2D, 0, gl.RGBA8, int32(width), int32(height), 0, gl.RGBA, gl.UNSIGNED_BYTE, nil)
        gl.FramebufferTexture2D(gl.FRAMEBUFFER, gl.COLOR_ATTACHMENT0, gl.TEXTURE_2D, colorTex, 0)
    } else {
        gl.BindTexture(gl.TEXTURE_2D_MULTISAMPLE, colorTex)
        //gl.TexParameteri(gl.TEXTURE_2D_MULTISAMPLE, gl.TEXTURE_WRAP_S, gl.REPEAT)
        //gl.TexParameteri(gl.TEXTURE_2D_MULTISAMPLE, gl.TEXTURE_WRAP_T, gl.REPEAT)
        //gl.TexParameteri(gl.TEXTURE_2D_MULTISAMPLE, gl.TEXTURE_MIN_FILTER, gl.NEAREST)
        //gl.TexParameteri(gl.TEXTURE_2D_MULTISAMPLE, gl.TEXTURE_MAG_FILTER, gl.NEAREST)
        gl.TexImage2DMultisample(gl.TEXTURE_2D_MULTISAMPLE, multiSample, gl.RGBA8, int32(width), int32(height), true)
        gl.FramebufferTexture2D(gl.FRAMEBUFFER, gl.COLOR_ATTACHMENT0, gl.TEXTURE_2D_MULTISAMPLE, colorTex, 0)
    }

    gl.GenRenderbuffers(1, &depthRb);
    gl.BindRenderbuffer(gl.RENDERBUFFER, depthRb)
    if multiSample == 0 {
        gl.RenderbufferStorage(gl.RENDERBUFFER, gl.DEPTH_COMPONENT24, int32(width), int32(height))
    } else {
        gl.RenderbufferStorageMultisample(gl.RENDERBUFFER, multiSample, gl.DEPTH_COMPONENT24, int32(width), int32(height))
    }

    gl.FramebufferRenderbuffer(gl.FRAMEBUFFER, gl.DEPTH_ATTACHMENT, gl.RENDERBUFFER, depthRb)

    status := gl.CheckFramebufferStatus(gl.FRAMEBUFFER)
    if status != gl.FRAMEBUFFER_COMPLETE {
        reason := ""
        switch(status) {
        case gl.FRAMEBUFFER_UNDEFINED:
            reason = "FRAMEBUFFER_UNDEFINED"
            break
        case gl.FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
            reason = "INCOMPLETE_ATTACHMENT"
            break
        case gl.FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
            reason = "INCOMPLETE_MISSING_ATTACHMENT"
            break
        case gl.FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:
            reason = "INCOMPLETE_DRAW_BUFFER"
            break
        case gl.FRAMEBUFFER_UNSUPPORTED:
            reason = "UNSUPPORTED";
            break
        case gl.FRAMEBUFFER_INCOMPLETE_MULTISAMPLE:
            reason = "INCOMPLETE_MULTISAMPLE"
            break
        case gl.FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS:
            reason = "INCOMPLETE_LAYER_TARGETS"
            break
        }
        println(reason);
        panic("Creating RenderTarget failed! ")
    }
    // restore state 
    gl.BindFramebuffer(gl.DRAW_FRAMEBUFFER, uint32(drawFboId))
    gl.BindFramebuffer(gl.READ_FRAMEBUFFER, uint32(readFboId))

    rt.fbo = fbo
    rt.depthRb = depthRb
    rt.colorTex = colorTex
    rt.Width = width
    rt.Height = height
    rt.multisample = multiSample
    mat4_ortho(&rt.screenM, float32(-width/2), float32(width/2), float32(-height/2), float32(height/2), -1.0, 1.0)
    return rt
}

func (r *RenderTarget) Destroy() {
    gl.DeleteFramebuffers(1, &r.fbo)
    gl.DeleteTextures(1, &r.colorTex)
    gl.DeleteRenderbuffers(1, &r.depthRb)
    if(r.msReadFbo != 0) {
        gl.DeleteFramebuffers(1, &r.msReadFbo)
        gl.DeleteTextures(1, &r.msReadColorTex)
    }
}

func (r *RenderTarget) GetPixels() []byte {
    buf := make([]byte, r.Width*r.Height*3)

    // save FBO state
    drawFboId, readFboId := int32(0), int32(0)
    gl.GetIntegerv(gl.DRAW_FRAMEBUFFER_BINDING, &drawFboId)
    gl.GetIntegerv(gl.READ_FRAMEBUFFER_BINDING, &readFboId)

    if(r.multisample == 0) {
        gl.BindFramebuffer(gl.READ_FRAMEBUFFER, r.fbo)
        gl.ReadPixels(0, 0, r.Width, r.Height, gl.RGB, gl.UNSIGNED_BYTE, unsafe.Pointer(&buf[0]))
    } else {
        // if the rendertarget is multisampled we can't use ReadPixels directly
        // thus we have to create intermediate buffer ro read from
        if(r.msReadFbo == 0) {
            // create non multisampled fbo to copy to

            var colorTex, fbo uint32
            gl.GenFramebuffers(1, &fbo)
            gl.BindFramebuffer(gl.FRAMEBUFFER, fbo)

            gl.GenTextures(1, &colorTex)
            gl.BindTexture(gl.TEXTURE_2D, colorTex)
            gl.TexParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_S, gl.REPEAT)
            gl.TexParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_T, gl.REPEAT)
            gl.TexParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.NEAREST)
            gl.TexParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.NEAREST)
            gl.TexImage2D(gl.TEXTURE_2D, 0, gl.RGBA8, int32(r.Width), int32(r.Height), 0, gl.RGBA, gl.UNSIGNED_BYTE, nil)
            gl.FramebufferTexture2D(gl.FRAMEBUFFER, gl.COLOR_ATTACHMENT0, gl.TEXTURE_2D, colorTex, 0)

            status := gl.CheckFramebufferStatus(gl.FRAMEBUFFER)
            if status != gl.FRAMEBUFFER_COMPLETE {
                panic("Creating read blit RenderTarget failed! ")
            }
            // restore state 
            r.msReadFbo = fbo;
            r.msReadColorTex = colorTex;
        }
        // blit multisampled fbo to non-multisampled one
        gl.BindFramebuffer(gl.READ_FRAMEBUFFER, r.fbo)
        gl.BindFramebuffer(gl.DRAW_FRAMEBUFFER, r.msReadFbo)
        gl.BlitFramebuffer(0, 0, r.Width, r.Height, 0, 0, r.Width, r.Height, gl.COLOR_BUFFER_BIT, gl.NEAREST)
        // read pixels from non multisampled fbo
        gl.BindFramebuffer(gl.READ_FRAMEBUFFER, r.msReadFbo)
        gl.ReadPixels(0, 0, r.Width, r.Height, gl.RGB, gl.UNSIGNED_BYTE, unsafe.Pointer(&buf[0]))
    }
    // restore framebuffer state
    gl.BindFramebuffer(gl.DRAW_FRAMEBUFFER, uint32(drawFboId))
    gl.BindFramebuffer(gl.READ_FRAMEBUFFER, uint32(readFboId))
    return buf
}

func (r *RenderState) compileShaders() {
    r.circleProgram = compileShader(circleVert, circleFrag, "")
    r.rectangleProgram = compileShader(rectangleVert, rectangleFrag, "")
    r.defaultProgram = compileShader(vertShader, fragShader, "")
    r.wireTriangleProgram = compileShader(wireTriangleVert, wireTriangleFrag, "")
    r.lineProgram = compileShader(lineVert, lineFrag, lineGeom)
    r.textProgram = compileShader(textVert, textFrag, "")
}

func (r *RenderState) createMeshes() {
    vData := [18]float32{
       -0.5, -0.5, 0.0,
        0.5, -0.5, 0.0,
        -0.5, 0.5, 0.0,
        0.5, -0.5, 0.0,
        0.5,  0.5, 0.0,
        -0.5, 0.5, 0.0,
    }
    gl.GenVertexArrays(1, &r.rectVao)
    gl.BindVertexArray(r.rectVao)
    var vbo uint32
    gl.GenBuffers(1, &vbo)
    gl.BindBuffer(gl.ARRAY_BUFFER, vbo)
    gl.BufferData(gl.ARRAY_BUFFER, len(vData)*4, unsafe.Pointer(&vData[0]), gl.STATIC_DRAW)
    gl.EnableVertexAttribArray(0)
    gl.VertexAttribPointer(0, 3, gl.FLOAT, false, 0, nil)
    gl.BindVertexArray(0)
}

func NewRenderState(width, height int32) *RenderState {
    r := new(RenderState)
    r.activeCam = nil
    // TODO: release shaders
    r.compileShaders()
    // TODO: release buffers
    r.createMeshes()
    InitFontRenderState(&r.fontState, "/usr/share/fonts/truetype/ubuntu/Ubuntu-B.ttf", 96)

    r.DefaultRenderTarget = RenderTarget{
        Width: width,
        Height: height,
        depthRb: 0,
        colorTex: 0,
        fbo: 0,
        // NOTE: screenM will be set by r.Resize()
    }
    r.renderTargets = append(r.renderTargets, &r.DefaultRenderTarget)

    /*for i := 0; i < 200; i++ {
        r.fontState.CacheGlyph(rune(i), 0)
    }
    r.fontState.debugOutputPng()*/
    r.Resize(width, height)
    gl.DebugMessageCallback(debugCallback, nil)
    gl.Enable(gl.DEBUG_OUTPUT)
    return r
}

func DestroyRenderState(r *RenderState) {
    DestroyFontRender(&r.fontState)
}

func (r *RenderState) CreateCamera(fov, zNear, zFar float32) *Camera {
    cam := new(Camera)
    cam.Fov = fov
    cam.ZNear = zNear
    cam.ZFar = zFar
    cam.AspectRatio = float32(r.width)/float32(r.height)
    cam.Pos = V3{0.0, 0.0, 0.0}
    cam.Rot = Quat{1.0, 0.0, 0.0, 0.0}
    cam.UpdateMatrices()
    cam.renderTarget = &r.DefaultRenderTarget;
    r.cameras = append(r.cameras, cam)
    r.activeCam = cam
    return cam
}

func (r *RenderState) Resize(width, height int32) {
    mat4_ortho(&r.DefaultRenderTarget.screenM, float32(-width/2), float32(width/2), float32(-height/2), float32(height/2), -1.0, 1.0)
    r.width = width
    r.height = height
    gl.Viewport(0, 0, width, height)
    for _, cam := range r.cameras {
        cam.AspectRatio = float32(width)/float32(height)
        cam.UpdateMatrices()
    }
}

func (r *RenderState) CreateCircle(pos V3, radius float32, color uint32, outline V4) {
    circle := new(Circle)
    circle.pos = pos
    circle.radius = radius
    circle.color = color
    circle.outline = outline
    r.circles = append(r.circles, circle)
}

func (r *RenderState) CreateRect(pos V3, rot Quat, width, height float32, color uint32, outline V4) *Rectangle {
    rect := new(Rectangle)
    rect.Pos = pos
    rect.Rot = rot
    rect.Width = width
    rect.Height = height
    rect.Color = color
    rect.Outline = outline
    r.rectangles = append(r.rectangles, rect)
    return rect
}

func (r *RenderState) CreateLinestrip(vertices []V3, colors []V4, width float32) *Linestrip {
    ls := new(Linestrip)
    vcpy := make([]V3, len(vertices))
    ccpy := make([]V4, len(colors))
    copy(vcpy, vertices)
    copy(ccpy, colors)
    ls.Vertices = vcpy
    ls.Colors = ccpy
    ls.Width = width
    ls.meshVao = 0
    r.linestrips = append(r.linestrips, ls)
    return ls
}

func (r *RenderState) CreateTriangleMesh(pos V3, triangles []Triangle) *TriangleMesh {
    tm := new(TriangleMesh)
    tm.Pos = pos;
    r.triangleMeshes = append(r.triangleMeshes, tm)

    vboData := make([]TriangleVertex, len(triangles)*3)
    for i := 0; i < len(triangles); i++ {
        vboData[i*3 + 0].pos = triangles[i].Pos[0]
        vboData[i*3 + 0].color = triangles[i].Color[0]
        vboData[i*3 + 0].bary = V3{1.0, 0.0, 0.0}
        vboData[i*3 + 1].pos = triangles[i].Pos[1]
        vboData[i*3 + 1].color = triangles[i].Color[1]
        vboData[i*3 + 1].bary = V3{0.0, 1.0, 0.0}
        vboData[i*3 + 2].pos = triangles[i].Pos[2]
        vboData[i*3 + 2].color = triangles[i].Color[2]
        vboData[i*3 + 2].bary = V3{0.0, 0.0, 1.0}
    }

    var vao, vbo uint32
    gl.GenVertexArrays(1, &vao)
    gl.BindVertexArray(vao)
    gl.GenBuffers(1, &vbo)
    gl.BindBuffer(gl.ARRAY_BUFFER, vbo)
    vertSize := int(unsafe.Sizeof(vboData[0]))
    vboSize := len(triangles)*3*vertSize // 3 vertices, 3 floats for pos, 4 floats for color
    gl.BufferData(gl.ARRAY_BUFFER, vboSize, unsafe.Pointer(&vboData[0]), gl.STATIC_DRAW)
    gl.EnableVertexAttribArray(0) // pos
    gl.VertexAttribPointer(0, 3, gl.FLOAT, false, int32(vertSize), nil)
    gl.EnableVertexAttribArray(1) // color
    gl.VertexAttribPointer(1, 4, gl.FLOAT, false, int32(vertSize), gl.PtrOffset(int(unsafe.Offsetof(vboData[0].color))))
    gl.EnableVertexAttribArray(2) // barycentric coords
    gl.VertexAttribPointer(2, 3, gl.FLOAT, false, int32(vertSize), gl.PtrOffset(int(unsafe.Offsetof(vboData[0].bary))))

    tm.vao = vao
    tm.vbo = vbo
    // TODO: copy?
    tm.triangles = triangles

    return tm;
}

func (r *RenderState) Render() {
    cam := r.activeCam
    rt := cam.renderTarget

    if rt == nil {
        cam.renderTarget = &r.DefaultRenderTarget
        rt = cam.renderTarget
    }
    gl.BindFramebuffer(gl.DRAW_FRAMEBUFFER, rt.fbo)
    // TODO: is there any point optimizing this?
    gl.Viewport(0, 0, rt.Width, rt.Height)
    gl.ClearColor(1.0, 1.0, 1.0, 1.0)
    gl.ClearDepth(1.0)
    gl.Clear(gl.COLOR_BUFFER_BIT|gl.DEPTH_BUFFER_BIT)


    cam.UpdateMatrices()

    r.renderCircles()
    r.renderRectangles()
    r.renderTriangleMeshes()
    r.renderLinestrips()

    for i := 0; i < 0; i++ {
        r.fontState.addString(V3{-400.0, 340.0 + -14.6*float32(i), 0.0}, "Everything he knew scattered into dust.", 11)
        r.fontState.addString(V3{0.0, 340.0 + -14.6*float32(i), 0.0}, "func (c *Camera) UpdateWorldToView() {.", 11)
    }
    r.fontState.addString(V3{-400.0, -69.2, 0.0}, "BIG text", 32)
    r.fontState.flushStrings(&rt.screenM, r.textProgram)

    if rt.OnRender != nil {
        rt.OnRender(rt)
    }
}

func (c *Camera) UpdateWorldToView() {
    invRot := c.Rot
    invRot.W *= -1
    invPos := V3{-c.Pos.X, -c.Pos.Y, -c.Pos.Z}
    mat4_rt(&c.WorldToView, invRot, invPos)
    mat4_mul(&c.WorldToClip, &c.ViewToClip, &c.WorldToView)
}

func (c *Camera) CalcViewToWorld(m *Mat4) {
    mat4_rt(m, c.Rot, c.Pos);
}

func (c *Camera) UpdateViewToClip() {
    mat4_perspective(&c.ViewToClip, c.Fov, c.AspectRatio, c.ZNear, c.ZFar)
}

func (c *Camera) UpdateMatrices() {
    c.UpdateViewToClip()
    c.UpdateWorldToView()
}

func (c *Camera) SetRenderTarget(rt *RenderTarget) {
    c.renderTarget = rt
    if rt != nil {
        c.AspectRatio = float32(rt.Width)/float32(rt.Height)
        c.UpdateMatrices()
        println("aspect ", c.AspectRatio)
    }
}

func (r *RenderState) renderCircles() {
    cam := r.activeCam
    if cam != nil && len(r.circles) > 0 {
        gl.UseProgram(r.circleProgram)
        loc := gl.GetUniformLocation(r.circleProgram, gl.Str("_ModelToClip\x00"))
        colLoc := gl.GetUniformLocation(r.circleProgram, gl.Str("_Color\x00"))
        outlineLoc := gl.GetUniformLocation(r.circleProgram, gl.Str("_Outline\x00"))
        gl.BindVertexArray(r.rectVao)
        gl.VertexAttrib4f(1, 1.0, 1.0, 1.0, 1.0)
        for _,x := range r.circles {
            var modelToWorld, modelToClip Mat4
            mat4_ts(&modelToWorld, x.pos, V3{x.radius*2.0, x.radius*2.0, x.radius*2.0})
            mat4_mul(&modelToClip, &cam.WorldToClip, &modelToWorld)
            gl.UniformMatrix4fv(loc, 1, false, &modelToClip.M[0][0])
            col4 := V4FromInt32(x.color)
            gl.Uniform4f(colLoc, col4.X, col4.Y, col4.Z, col4.W)
            b := x.outline
            gl.Uniform4f(outlineLoc, b.X, b.Y, b.Z, b.W)
            gl.DrawArrays(gl.TRIANGLES, 0, 6)
        }
    }
}

func (r *RenderState) renderRectangles() {
    cam := r.activeCam
    if cam != nil && len(r.rectangles) > 0{
        gl.UseProgram(r.rectangleProgram)
        loc := gl.GetUniformLocation(r.rectangleProgram, gl.Str("_ModelToClip\x00"))
        outlineLoc := gl.GetUniformLocation(r.rectangleProgram, gl.Str("_Outline\x00"))
        colLoc := gl.GetUniformLocation(r.rectangleProgram, gl.Str("_Color\x00"))
        gl.BindVertexArray(r.rectVao)
        gl.VertexAttrib4f(1, 1.0, 1.0, 1.0, 1.0)
        for _,x := range r.rectangles {
            var modelToWorld, modelToClip Mat4
            mat4_trs(&modelToWorld, x.Pos, x.Rot, V3{x.Width, x.Height, 1.0})
            mat4_mul(&modelToClip, &cam.WorldToClip, &modelToWorld)
            gl.UniformMatrix4fv(loc, 1, false, &modelToClip.M[0][0])
            bb := x.Outline
            gl.Uniform4f(outlineLoc, bb.X, bb.Y, bb.Z, bb.W)
            col4 := V4FromInt32(x.Color)
            gl.Uniform4f(colLoc, col4.X, col4.Y, col4.Z, col4.W)
            gl.DrawArrays(gl.TRIANGLES, 0, 6)
        }
    }
}

func (r *RenderState) renderTriangleMeshes() {
    cam := r.activeCam
    if cam != nil && len(r.triangleMeshes) > 0 {
        prog := r.wireTriangleProgram
        gl.UseProgram(prog)
        loc := gl.GetUniformLocation(prog, gl.Str("_ModelToClip\x00"))
        outlineLoc := gl.GetUniformLocation(prog, gl.Str("_Outline\x00"))
        colLoc := gl.GetUniformLocation(prog, gl.Str("_Color\x00"))
        gl.VertexAttrib4f(1, 1.0, 1.0, 1.0, 1.0)
        for _, tm := range r.triangleMeshes {
            // TODO: create vao
            gl.BindVertexArray(tm.vao)
            var modelToWorld, modelToClip Mat4
            mat4_trs(&modelToWorld, tm.Pos, tm.Rot, V3{1.0, 1.0, 1.0})
            mat4_mul(&modelToClip, &cam.WorldToClip, &modelToWorld)
            gl.UniformMatrix4fv(loc, 1, false, &modelToClip.M[0][0])
            gl.Uniform4f(outlineLoc, 0.0, 0.0, 0.0, 0.0)
            gl.Uniform4f(colLoc, 1.0, 1.0, 1.0, 1.0)
            gl.DrawArrays(gl.TRIANGLES, 0, int32(len(tm.triangles)*3))
        }
    }
}

func (r *RenderState) renderLinestrips() {

    cam := r.activeCam
    if cam != nil && len(r.linestrips) > 0 {
        gl.Disable(gl.CULL_FACE)
        gl.UseProgram(r.lineProgram)
        m2v := gl.GetUniformLocation(r.lineProgram, gl.Str("_ModelToView\x00"))
        v2c := gl.GetUniformLocation(r.lineProgram, gl.Str("_ViewToClip\x00"))
        outlineLoc := gl.GetUniformLocation(r.lineProgram, gl.Str("_Outline\x00"))
        colLoc := gl.GetUniformLocation(r.lineProgram, gl.Str("_Color\x00"))
        widthLoc := gl.GetUniformLocation(r.lineProgram, gl.Str("_LineWidth\x00"))
        gl.UniformMatrix4fv(m2v, 1, false, &cam.WorldToView.M[0][0])
        gl.UniformMatrix4fv(v2c, 1, false, &cam.ViewToClip.M[0][0])
        var viewToWorld Mat4
        cam.CalcViewToWorld(&viewToWorld)
        gl.VertexAttrib4f(1, 1.0, 1.0, 1.0, 1.0)
        for _,x := range r.linestrips {
            if x.meshVao == 0 {
                x.Update()
            }
            gl.BindVertexArray(x.meshVao)
            bb := x.Outline
            gl.Uniform4f(outlineLoc, bb.X, bb.Y, bb.Z, bb.W)
            //col4 := V4FromInt32(x.Color)
            //gl.Uniform4f(colLoc, col4.X, col4.Y, col4.Z, col4.W)
            gl.Uniform4f(colLoc, 1.0, 1.0, 1.0, 1.0)
            gl.Uniform1f(widthLoc, x.Width)
            //gl.DrawArrays(gl.LINE_STRIP, 0, int32(len(x.meshVertices)))
            gl.DrawElements(gl.LINE_STRIP_ADJACENCY, int32(len(x.meshIndices)), gl.UNSIGNED_SHORT, gl.PtrOffset(0))
        }
    }
}
