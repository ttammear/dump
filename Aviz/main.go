package main

import . "./svrend"
import "github.com/veandco/go-sdl2/sdl"
import "fmt"
import "math"
//TODO

func main() {
    var svr Svrend
    svr.Init("Aviz", 1024, 768)

    renderer := svr.Renderer

    cam := renderer.CreateCamera(90.0, 0.1, 1000.0)
    cam.Pos = V3{0.0, 0.0, -5.0}
    cam.UpdateMatrices()

    renderer.CreateCircle(V3{1.0, 2.0, 10.0}, 2.0, 0xFF0F00FF, V4{0.0, 0.0, 0.0, 10.0})
    renderer.CreateRect(V3{-2.0, 1.0, 0.0}, Z2d(45.0), 1.0, 1.0, 0x0000FF8F, V4{0.0, 0.0, 0.0, 3.0})
    for i := 0; i < 32; i++ {
        for j := 0; j < 32; j++ {
            renderer.CreateRect(V3{float32(i)*1.41415, float32(j)*1.41415, 0.0}, Z2d(45.0), 1.0, 1.0, 0x0000FFFF, V4{0.0, 0.0, 0.0, 3.0})
        }
    }
    rect := renderer.CreateRect(V3{-8.0, 0.0, 0.01}, Quat{1.0, 0.0, 0.0, 0.0}, 1.0, 8.0, 0xFF00FFFF, V4{0.0, 0.0, 0.0, 1.0})

    mtris := make([]Triangle, 1)
    mtris[0] = Triangle{
        Pos:[3]V3{
            V3{0.0, 0.0, 0.0},
            V3{0.5, 1.0, 0.0},
            V3{1.0, 0.0, 0.0},
        },
        Color:[3]V4{
            V4{1.0, 0.0, 0.0, 1.0},
            V4{0.0, 1.0, 0.0, 1.0},
            V4{0.0, 0.0, 1.0, 1.0},
        },
    };
    renderer.CreateTriangleMesh(V3{-10.0, 0.0, 0.0}, mtris)

    numSegs := 150
    lsV := make([]V3, numSegs)
    lsC := make([]V4, numSegs)
    for i := 0; i < len(lsV); i++ {
        lsV[i] = V3{float32(i)*0.05, -4.0, 0.0}
        lsC[i] = V4{0.0, 1.0,  0.0, 1.0}
    }
    lsV3 := make([]V3, numSegs)
    lsC3 := make([]V4, numSegs)
    for i := 0; i < len(lsV3); i++ {
        lsV3[i] = V3{0.0, float32(i)*0.05, 0.0}
        lsC3[i] = V4{1.0, 0.0,  0.0, 1.0}
    }

    lsV4 := make([]V3, 362)
    lsC4 := make([]V4, 362)
    for i := 0; i < len(lsV4); i++ {
        lsV4[i] = V3{float32(math.Sin(float64(i)*(math.Pi/180.0))), float32(math.Cos(float64(i)*(math.Pi/180.0))) - 5.0, 0.0}
        lsC4[i] = V4{1.0, 0.0,  0.0, 1.0}
    }

    lsV2 := []V3{V3{0.0, -1.0, 0.0}, V3{1.0, 2.0, 0.0}, V3{1.0, -2.0, 0.0}, V3{3.0, -2.0, 0.0}, V3{3.0, -5.0, 0.0}, V3{4.0, -5.0, 0.0}, V3{5.0, -5.0, 0.0}}
    lsC2 := []V4{V4{1.0, 0.0, 0.0, 1.0}, V4{1.0, 0.0, 0.0, 1.0}, V4{1.0, 0.0, 0.0, 1.0}, V4{1.0, 0.0, 0.0, 1.0}, V4{1.0, 1.0, 0.0, 1.0}, V4{1.0, 1.0, 0.0, 1.0}, V4{1.0, 1.0, 0.0, 1.0}}


    lineStrip := renderer.CreateLinestrip(lsV, lsC, 0.1)
    lineStrip2 := renderer.CreateLinestrip(lsV3, lsC3, 0.1)
    renderer.CreateLinestrip(lsV4, lsC4, 0.1)
    renderer.CreateLinestrip(lsV2, lsC2, 0.1)

    // Set up ffmpeg
    rt := renderer.CreateRenderTarget(1920, 1080, 4)
    //cam.SetRenderTarget(rt)
    var ffmpeg FfmpegCtx
    ffmpeg.start("foo2.mp4", rt.Width, rt.Height)
    count := 0
    rt.OnRender = func (rt *RenderTarget) {
        println("Frame: ", count);
        testData := rt.GetPixels()
        ffmpeg.sendFrame(testData)
        renderer.Blit(rt, &renderer.DefaultRenderTarget)
        count++
    }

    camR := V3{0.0, 0.0, 0.0};

    t := float32(0.0)
    //fontMain()
    oldTime := sdl.GetPerformanceCounter()
    running := true
    totalTime := float64(0.0)
    println("Starting rendering")
    for running {
        curTime := sdl.GetPerformanceCounter()
        realDt := float32(curTime-oldTime)/float32(sdl.GetPerformanceFrequency())
        dt := float32(1.0/60.0)
        //dt := realDt
        oldTime = curTime
        realDtMs := realDt*1000.0
        fmt.Printf("Frametime: %.2fms\n", realDtMs)

        totalTime += float64(dt)

        running = svr.DoEvents()

        for i := 0; i < len(lsV); i++ {
            lineStrip.Vertices[i].Y = float32(math.Sin(totalTime+float64(i)*0.05))
            lineStrip.Vertices[i].Z = float32(math.Sin(totalTime+float64(i)*0.05))
        }
        lineStrip.Update()
        for i := 0; i < len(lsV3); i++ {
            lineStrip2.Vertices[i].X = float32(math.Sin(totalTime+float64(i)*0.05))
        }
        lineStrip2.Update()

        kstate := sdl.GetKeyboardState()
        moveAmount := dt
        if kstate[sdl.SCANCODE_Q] != 0 {
            cam.Pos.Z += moveAmount*10
            cam.UpdateWorldToView()
        }
        if kstate[sdl.SCANCODE_E] != 0 {
            cam.Pos.Z -= moveAmount*10
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

        if kstate[sdl.SCANCODE_1] != 0 {
            camR.X += dt;
        }
        if kstate[sdl.SCANCODE_2] != 0 {
            camR.X -= dt;
        }
        if kstate[sdl.SCANCODE_3] != 0 {
            camR.Y += dt;
        }
        if kstate[sdl.SCANCODE_4] != 0 {
            camR.Y -= dt;
        }
        if kstate[sdl.SCANCODE_5] != 0 {
            camR.Z += dt;
        }
        if kstate[sdl.SCANCODE_6] != 0 {
            camR.Z -= dt;
        }

        t += dt
        rect.Rot = Z2d(t*60.0)
        cam.Rot = Quat_mul(Quat_mul(Z2d(camR.X*60.0), Y2d(camR.Y*60.0)), X2d(camR.Z*60.0));

        svr.RenderAndPush()
    }

    rt.Destroy()

    ffmpeg.stop()
    ffmpeg.wait()

    svr.Destroy()
}
