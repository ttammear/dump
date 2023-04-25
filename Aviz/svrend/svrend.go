package svrend

import "github.com/veandco/go-sdl2/sdl"
import gl "github.com/go-gl/gl/v3.3-core/gl"

type Svrend struct {
    Window *sdl.Window
    GlCtx sdl.GLContext
    Renderer *RenderState
    oldTime uint64
}

func (s *Svrend) Init(title string, width int32, height int32) {
    var err error
    if err := sdl.Init(sdl.INIT_VIDEO); err != nil {
        panic(err)
    }
    s.Window, err = sdl.CreateWindow(title, sdl.WINDOWPOS_UNDEFINED, sdl.WINDOWPOS_UNDEFINED, width, height, sdl.WINDOW_SHOWN|sdl.WINDOW_OPENGL)
    if err != nil {
        panic(err)
    }
    sdl.GLSetAttribute(sdl.GL_CONTEXT_MAJOR_VERSION, 3)
    sdl.GLSetAttribute(sdl.GL_CONTEXT_MINOR_VERSION, 3)
    sdl.GLSetAttribute(sdl.GL_CONTEXT_PROFILE_MASK, sdl.GL_CONTEXT_PROFILE_CORE)
    sdl.GLSetAttribute(sdl.GL_MULTISAMPLESAMPLES, 4)
    sdl.GLSetSwapInterval(1)
    context, err := s.Window.GLCreateContext()
    if err != nil {
        panic(err)
    }
    s.GlCtx = context
    gl.Init()
    gl.Viewport(0, 0, width, height)
    gl.Enable(gl.BLEND)
    gl.BlendEquationSeparate(gl.FUNC_ADD, gl.FUNC_ADD);
    gl.BlendFuncSeparate(gl.ONE, gl.ONE_MINUS_SRC_ALPHA, gl.ONE, gl.ZERO)

    gl.Enable(gl.CULL_FACE)
    gl.FrontFace(gl.CCW)
    gl.CullFace(gl.BACK)

    gl.DepthFunc(gl.LEQUAL)
    gl.Enable(gl.DEPTH_TEST)

    s.Renderer = NewRenderState(width, height)

    s.oldTime = sdl.GetPerformanceCounter()
}

func (s *Svrend) Destroy() {
    sdl.GLDeleteContext(s.GlCtx)
    s.Window.Destroy()
    // TODO: ???
    sdl.Quit()
}

func (s *Svrend) DoEvents() bool {
    ret := true
    for event := sdl.PollEvent(); event != nil; event = sdl.PollEvent() {
        switch t := event.(type) {
        case *sdl.QuitEvent:
            ret = false
            break
        case *sdl.WindowEvent:
            wevent := t.Event
            switch wevent {
            case sdl.WINDOWEVENT_RESIZED:
                s.Renderer.Resize(t.Data1, t.Data2)
                break
            }
            break;
        }
    }
    return ret
}

func (s *Svrend) RenderAndPush() {
    s.Renderer.Render()
    s.Window.GLSwap()
}
