package main

import . "../svrend"
import "math/rand"
import "github.com/veandco/go-sdl2/sdl"


type App struct {
    svr Svrend
    renderer *RenderState

    packState RectPackState
}

func (app *App) addRectangle(c IV2, s IV2) {
    r := app.renderer
    r.CreateRect(V3{-500+float32(c.X)+float32(s.X)/2, -500+float32(c.Y)+float32(s.Y)/2, 0.0}, Quat{1.0, 0.0, 0.0, 0.0}, float32(s.X), float32(s.Y), 0xFF0000FF,V4{0.0, 0.0, 0.0, 0.0} )
}

func (a *App) packerTest() {
    var rps RectPackState
    InitRectPackState(&rps, 1000, 1000)
    for i := 0; i < 200; i++ {
        size := IV2{rand.Int31n(30)+30, rand.Int31n(60)+40}
        pos := rps.PlaceRectangle(size)
        a.addRectangle(pos, size)
    }
}

func main() {
    var svr Svrend
    svr.Init(1024, 768)


    renderer := svr.Renderer

    cam := renderer.CreateCamera(90.0, 0.1, 10000.0)
    cam.Pos = V3{0.0, 0.0, -800.0}
    cam.UpdateMatrices()

    var app App
    app.svr = svr
    app.renderer = renderer

    //app.addRectangle(V2{0.0, 0.0}, V2{100, 200})
    app.packerTest()

    renderer.CreateCircle(V3{1.0, 2.0, 10.0}, 2.0, 0xFF0F00FF, V4{0.0, 0.0, 0.0, 10.0})
    renderer.CreateLinestrip([]V3{V3{-500.0, -500.0, 0.0}, V3{500.0, -500.0, 0.0}}, []V4{V4{0.0, 0.0, 0.0, 1.0}, V4{0.0, 0.0, 0.0, 1.0}}, 1.0);
    renderer.CreateLinestrip([]V3{V3{-500.0, 500.0, 0.0}, V3{500.0, 500.0, 0.0}}, []V4{V4{0.0, 0.0, 0.0, 1.0}, V4{0.0, 0.0, 0.0, 1.0}}, 1.0);
    renderer.CreateLinestrip([]V3{V3{-500.0, -500.0, 0.0}, V3{-500.0, 500.0, 0.0}}, []V4{V4{0.0, 0.0, 0.0, 1.0}, V4{0.0, 0.0, 0.0, 1.0}}, 1.0);
    renderer.CreateLinestrip([]V3{V3{500.0, -500.0, 0.0}, V3{500.0, 500.0, 0.0}}, []V4{V4{0.0, 0.0, 0.0, 1.0}, V4{0.0, 0.0, 0.0, 1.0}}, 1.0);

    oldTime := sdl.GetPerformanceCounter()

    running := true
    totalTime := float64(0.0)
    for running {
        curTime := sdl.GetPerformanceCounter()
        dt := float32(curTime-oldTime)/float32(sdl.GetPerformanceFrequency())
        oldTime = curTime

        totalTime += float64(dt)

        running = svr.DoEvents()

        //var dt float32 = 1/60.0

        kstate := sdl.GetKeyboardState()
        moveAmount := dt*100
        if kstate[sdl.SCANCODE_Q] != 0 {
            cam.Pos.Z += moveAmount
            cam.UpdateWorldToView()
        }
        if kstate[sdl.SCANCODE_E] != 0 {
            cam.Pos.Z -= moveAmount
            cam.UpdateWorldToView()
        }
        if kstate[sdl.SCANCODE_W] != 0 {
            cam.Pos.Y += moveAmount
            cam.UpdateWorldToView()
        }
        if kstate[sdl.SCANCODE_S] != 0 {
            cam.Pos.Y -= moveAmount
            cam.UpdateWorldToView()
        }
        if kstate[sdl.SCANCODE_D] != 0 {
            cam.Pos.X += moveAmount
            cam.UpdateWorldToView()
        }
        if kstate[sdl.SCANCODE_A] != 0 {
            cam.Pos.X -= moveAmount
            cam.UpdateWorldToView()
        }

        svr.RenderAndPush()
    }
    svr.Destroy()
}
