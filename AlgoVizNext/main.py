import sys
import numpy
import math

#OpenGL.FULL_LOGGING = True # causes 3x higher CPU usage :(
from OpenGL.GL import *

import sdl2.ext
from sdl2 import video

from avizmath import *
from renderer import *

# NOTE: use numpy for buffer arrays

def main():
    if sdl2.SDL_Init(sdl2.SDL_INIT_VIDEO) != 0:
        print(sdl2.SDL_GetError())
        return -1
    window = sdl2.SDL_CreateWindow(b"AlgoVizNext", sdl2.SDL_WINDOWPOS_UNDEFINED, sdl2.SDL_WINDOWPOS_UNDEFINED, 1024, 768, sdl2.SDL_WINDOW_OPENGL)
    video.SDL_GL_SetAttribute(video.SDL_GL_CONTEXT_MAJOR_VERSION, 3)
    video.SDL_GL_SetAttribute(video.SDL_GL_CONTEXT_MINOR_VERSION, 3)
    video.SDL_GL_SetAttribute(video.SDL_GL_CONTEXT_PROFILE_MASK, video.SDL_GL_CONTEXT_PROFILE_CORE)
    video.SDL_GL_SetSwapInterval(1)
    context = sdl2.SDL_GL_CreateContext(window)
    if context == None:
        print(sdl2.SDL_GetError())
        return -1

    renderer = Renderer(1024.0, 768.0)
    cam = renderer.createCamera(90, 0.1, 1000.0)
    cam.setPos(V3(0, 0, -5.0))

    for i in range(32):
        for j in range(32):
            rect = renderer.createRect2(V3(i*1.414, j*1.414, 0.0), Quat.z2d(45.0), 1.0, 1.0, 0x0000FFFF, V4(0.0, 0.0, 0.0, 3.0))
    rect = renderer.createRect2(V3(0.0, 1.0, 0.0), Quat.z2d(45.0), 1.0, 1.0, 0x0000FF8F, V4(0.0, 0.0, 0.0, 3.0))
    rect2 = renderer.createRect2(V3(0.0, -1.0, 0.0), Quat.id, 1.0, 8.0, 0xFF00FFFF, V4(0.0, 0.0, 0.0, 1.0))
    rect3 = renderer.createRect(V3(0.0, -3.0, 0.0), Quat.id, 1.0, 6.0, 0xFF00FFFF)
    c2 = renderer.createCircle(V3(1.0, 2.0, 10.0), 2.0, 0xFF0F00FF, V4(0.0, 0.0, 0.0, 10.0))
    #renderer.createCircle(V3(1.0, 2.0, 0.0), 1.0, 0xFF0F00FF)
    #line = renderer.createLine(V3(-4.0, 0.0, 0.0), V3(4.0, 0.5, 0.0), 1.0, 0xFF00008F)
    #line2 = renderer.createLine(V3(-4.0, -1.0, 0.0), V3(4.0, -0.5, 0.0), 2.0, 0xFF00008F)

    t = 0.0
    '''sineLine = renderer.createLineStrip(5.0)
    for i in range(50):
        sineLine.addVertex(V3(0.0, 0.0, 0.0))'''

    glBlendEquationSeparate(GL_FUNC_ADD, GL_FUNC_ADD);
    glBlendFuncSeparate(GL_ONE, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ZERO);
    glEnable(GL_BLEND)
    # TODO: this feature is very unreliable, don't use it
    glEnable(GL_LINE_SMOOTH)
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST)

    running = True
    event = sdl2.SDL_Event()
    oldTime = sdl2.SDL_GetPerformanceCounter()
    while running:
        curTime = sdl2.SDL_GetPerformanceCounter()
        dt = (curTime-oldTime)/sdl2.SDL_GetPerformanceFrequency()
        dtms = dt*1000
        print(f"took {dtms}ms ({1/dt:.0f}FPS)")
        oldTime = curTime
        while sdl2.SDL_PollEvent(ctypes.byref(event)) != 0:
            if event.type == sdl2.SDL_QUIT:
                running = False
                break
            if event.type == sdl2.SDL_WINDOWEVENT:
                wevent = event.window.event
                if wevent == sdl2.SDL_WINDOWEVENT_RESIZED:
                    renderer.resize(event.window.data1, event.window.data2)
        glClearColor(1, 1, 1, 1)
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT)

        t += 1.0/60.0

        keystates = sdl2.SDL_GetKeyboardState(None)
        moveAmount = dt
        if keystates[sdl2.SDL_SCANCODE_Q]:
            cam.pos += V3(0.0, 0.0, moveAmount)
            cam.updateWorldToView()
        if keystates[sdl2.SDL_SCANCODE_E]:
            cam.pos -= V3(0.0, 0.0, moveAmount)
            cam.updateWorldToView()
        if keystates[sdl2.SDL_SCANCODE_W]:
            cam.pos += V3(0.0, moveAmount, 0.0)
            cam.updateWorldToView()
        if keystates[sdl2.SDL_SCANCODE_S]:
            cam.pos -= V3(0.0, moveAmount, 0.0)
            cam.updateWorldToView()
        if keystates[sdl2.SDL_SCANCODE_A]:
            cam.pos -= V3(moveAmount, 0.0, 0.0)
            cam.updateWorldToView()
        if keystates[sdl2.SDL_SCANCODE_D]:
            cam.pos += V3(moveAmount, 0.0, 0.0)
            cam.updateWorldToView()

        rect.rot = Quat.z2d(t*2100.0)
        rect2.rot = Quat.z2d(t*20.0)
        rect2.updateModelToWorld()
        rect3.rot = Quat.z2d(t*20.0)
        redRGB = V3(1.0, 0.0, 0.0)
        greenRGB = V3(0.0, 1.0, 0.0)
        '''for i in range(50):
            sineLine.vertices[i][0] = (i-25)*0.25
            sval = math.sin(i*0.25+t*1)
            sineLine.vertices[i][1] = sval
            colorRGB = V3.lerp(greenRGB, redRGB, abs(sval))
            sineLine.colors[i] = rgbToInt32(colorRGB)'''
        renderer.render()

        sdl2.SDL_GL_SwapWindow(window)

    sdl2.SDL_GL_DeleteContext(context)
    sdl2.SDL_DestroyWindow(window)
    sdl2.SDL_Quit()
    return 0

if __name__ == "__main__":
    main()
