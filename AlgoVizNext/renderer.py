from avizmath import *
from OpenGL.GL import *
from OpenGL.GL import shaders
from OpenGL.GL import extensions

vertShader = """#version 330 core 
layout(location = 0) in vec4 position;
layout(location = 1) in vec4 color;
out vec4 v_color;
uniform mat4 _ModelToClip;
void main() {
    gl_Position = _ModelToClip*position;
    v_color = color;
}"""

fragShader = """#version 330 core
layout(location = 0) out vec4 outColor;
in vec4 v_color;
uniform vec4 _Color;
void main() {
    vec3 outColorRGB = _Color.rgb*v_color.rgb;
    float alpha = _Color.a*v_color.a;
    outColor = vec4(outColorRGB*alpha, alpha);
}"""

rectangleVert = """#version 330 core
layout(location = 0) in vec4 position;
layout(location = 1) in vec4 color;
out vec4 v_color;
out vec3 v_modelPos;
uniform mat4 _ModelToClip;
void main() {
    gl_Position = _ModelToClip*position;
    v_modelPos = position.xyz;
    v_color = color;
}"""

rectangleFrag = """#version 330 core
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
}"""

circleVert = """#version 330 core 
layout(location = 0) in vec4 position;
layout(location = 1) in vec4 color;
out vec4 v_color;
out vec3 v_modelPos;
uniform mat4 _ModelToClip;
void main() {
    gl_Position = _ModelToClip*position;
    v_modelPos = position.xyz;
    v_color = color;
}"""
circleFrag = """#version 330 core
layout(location = 0) out vec4 outColor;
in vec4 v_color;
in vec3 v_modelPos;
uniform vec4 _Color;
uniform vec4 _Outline;
void main() {
    vec2 dx = dFdx(v_modelPos.xy);
    // TODO: non uniform scaling doesn't work (need derivative on y axis)
    float blendPrec = 1.0 - 4.0*length(dx);
    const float radius = 0.5;
    vec3 colRGB;
    float alpha;
    float dist = length(v_modelPos);
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
}
"""
class Renderer:
    def createBuiltinMeshes(self):
        # create square mesh
        vData = numpy.array([
            -0.5, -0.5, 0.0,
            0.5, -0.5, 0.0,
            -0.5, 0.5, 0.0,
            0.5, -0.5, 0.0,
            0.5,  0.5, 0.0,
            -0.5, 0.5, 0.0], dtype=numpy.float32)
        self.vao = glGenVertexArrays(1)
        glBindVertexArray(self.vao)
        vbo = glGenBuffers(1)
        glBindBuffer(GL_ARRAY_BUFFER, vbo)
        glBufferData(GL_ARRAY_BUFFER, vData.nbytes, vData, GL_STATIC_DRAW)
        glEnableVertexAttribArray(0)
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, ctypes.c_void_p(0))
        glBindVertexArray(0)

    def compileShaders(self):
        # TODO cleanup
        glVs = shaders.compileShader(vertShader, GL_VERTEX_SHADER)
        glFs = shaders.compileShader(fragShader, GL_FRAGMENT_SHADER)
        self.program = shaders.compileProgram(glVs, glFs)
        circleVs = shaders.compileShader(circleVert, GL_VERTEX_SHADER)
        circleFs = shaders.compileShader(circleFrag, GL_FRAGMENT_SHADER)
        self.circleProgram = shaders.compileProgram(circleVs, circleFs)
        rectangleVs = shaders.compileShader(rectangleVert, GL_VERTEX_SHADER)
        rectangleFs = shaders.compileShader(rectangleFrag, GL_FRAGMENT_SHADER)
        self.rectangleProgram = shaders.compileProgram(rectangleVs, rectangleFs)
    def requireExtension(self, extStr):
        if not extensions.hasGLExtension(extStr):
            raise Exception(f"This library requires {extStr} extension, but it is not supported")
    def getExtensions(self):
        self.requireExtension("GL_ARB_buffer_storage")
        buf = glGenBuffers(1)
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, buf)

    def __init__(self, width, height):
        self.getExtensions()
        self.cameras = []
        self.activeCam = None
        self.compileShaders()
        self.createBuiltinMeshes()
        self.circleRenderer = CircleRenderer(self.circleProgram, self)
        self.circleRenderer2 = CircleRenderer2(self.program, self)
        self.rectangleRenderer = RectangleRenderer(self.program, self)
        self.rectangleRenderer2 = RectangleRenderer2(self.rectangleProgram, self)
        self.lineRenderer = LineRenderer(self.program, self)
        self.width = width
        self.height = height
        self.curLine = None

    def __del__(self):
        glBindVertexArray(self.vao)
        vbo = glGetIntegerv(GL_ARRAY_BUFFER_BINDING)
        glDeleteBuffers(1, [vbo])

    def createCamera(self, fov, zNear, zFar):
        cam = Camera(self, fov, zNear, zFar, self.width, self.height)
        self.cameras.append(cam)
        # TODO: this isn't quite right
        self.activeCam = cam
        return cam

    def resize(self, newW, newH):
        self.width = newW
        self.height = newH
        for cam in self.cameras:
            cam.setTargetSize(newW, newH)
        glViewport(0, 0, newW, newH)

    def render(self):
        self.circleRenderer.render()
        self.circleRenderer2.render()
        self.rectangleRenderer.render()
        self.rectangleRenderer2.render()
        self.lineRenderer.render()

    def createCircle2(self, pos, radius, color=0xFFFFFFFF):
        circle = Circle(pos, radius, color)
        self.circleRenderer2.circles.append(circle)
        return circle

    def createCircle(self, pos, radius, color=0xFFFFFFFF, outline=V4(0.0, 0.0, 0.0, 0.0)):
        circle = BCircle(pos, radius, color, outline)
        self.circleRenderer.circles.append(circle)
        return circle

    def createRect(self, pos, rot, width, height, color=0xFFFFFFFF):
        rectangle = Rectangle(pos, rot, width, height, color)
        self.rectangleRenderer.rectangles.append(rectangle)
        return rectangle

    def createRect2(self, pos, rot, width, height, color=0xFFFFFFFF, border=V4(0.0, 0.0, 0.0, 0.0)):
        rectangle = BRectangle(pos, rot, width, height, color, border)
        self.rectangleRenderer2.rectangles.append(rectangle)
        return rectangle

    # n vertex line
    def createLineStrip(self, width=1.0):
        ret = LineStrip(width)
        self.lineRenderer.lineStrips.append(ret)
        return ret
    # 2 point line
    def createLine(self, startPos, endPos, width=1.0, color=0xFFFFFFFF):
        ret = LineStrip(width)
        ret.addVertex(startPos, color)
        ret.addVertex(endPos, color)
        self.lineRenderer.lineStrips.append(ret)
        return ret

    def destroy(self, obj):
        if type(obj) is BRectangle:
            self.rectangleRenderer2.rectangles.remove(obj)
        if type(obj) is Rectangle:
            self.rectangleRenderer.rectangles.remove(obj)
        if type(obj) is Circle:
            self.circleRenderer2.circles.remove(obj)
        if type(obj) is BCircle:
            self.circleRenderer.circles.remove(obj)

# TODO: rethink if you really need a border baked into shader (could just draw 2 circles)
class Circle:
    # Note: outline is (r, g, b, precentage) where 0.5 would be half of the circle radius
    def __init__(self, pos, radius, color):
        self.pos = pos
        self.radius = radius
        self.color = color
class BCircle(Circle):
    def __init__(self, pos, radius, color, outline):
        Circle.__init__(self, pos, radius, color)
        self.outline = outline

class Rectangle:
    def __init__(self, pos, rot, width, height, color=0xFFFFFFFF):
        self.pos = pos
        self.rot = rot
        self.width = width
        self.height = height
        self.color = color
        self.modelToWorld = numpy.empty((4, 4), dtype=numpy.float32)
        self.updateModelToWorld()
    def updateModelToWorld(self):
       mat4_trs(self.modelToWorld, self.pos, self.rot, V3(self.width, self.height, 1.0))
class BRectangle(Rectangle):
    def __init__(self, pos, rot, width, height, color=0xFFFFFFFF, border=V4(0.0, 0.0, 0.0, 0.0)):
        Rectangle.__init__(self, pos, rot, width, height, color)
        self.border = border
class LineStrip:
    def __init__(self, width):
        self.width = width
        self.vertices = []
        self.colors = []
    def addVertex(self, pos, color=0xFFFFFFFF):
        self.vertices.append([pos.x, pos.y, pos.z])
        self.colors.append(color)
class Camera:
    def __init__(self, renderer, fov, zNear, zFar, targetWidth, targetHeight):
        self.renderer = renderer
        self.fov = fov
        self.zNear = zNear
        self.zFar = zFar
        self.pos = V3(0.0, 0.0, 0.0)
        self.rotation = Quat(1.0, 0.0, 0.0, 0.0)
        self.targetWidth = targetWidth
        self.targetHeight = targetHeight
        self.viewToClip = numpy.zeros((4, 4), dtype=numpy.float32)
        self.worldToView = numpy.zeros((4, 4), dtype=numpy.float32)
        self.updateViewToClip()
        self.updateWorldToView()
    def updateWorldToClip(self):
        self.worldToClip = numpy.matmul(self.viewToClip, self.worldToView)
    def updateViewToClip(self):
        mat4_perspective(self.viewToClip, self.fov, self.targetWidth/self.targetHeight, self.zNear, self.zFar)
        self.updateWorldToClip()
    def updateWorldToView(self):
        invRot = self.rotation
        invRot.w = -invRot.w
        invPos = V3(-self.pos.x, -self.pos.y, -self.pos.z)
        self.worldToView = mat4_rt(invRot, invPos)
        self.updateWorldToClip()
    def setPos(self, pos):
        self.pos = pos
        self.updateWorldToView()
    def setRot(self, rot):
        self.rotation = rot
        self.updateWorldToView()
    def setTargetSize(self, targetWidth, targetHeight):
        self.targetWidth = targetWidth
        self.targetHeight = targetHeight
        self.updateViewToClip()

class RectangleRenderer:
    def __init__(self, program, renderer):
        self.program = program
        self.renderer = renderer
        self.rectangles = []
        # to avoid runtime allocation
        self.modelToWorld = numpy.empty((4, 4), dtype=numpy.float32)
        self.modelToClip = numpy.empty((4, 4), dtype=numpy.float32)

    def render(self):
        cam = self.renderer.activeCam
        if cam is None:
            return
        glUseProgram(self.program)
        loc = glGetUniformLocation(self.program, b"_ModelToClip")
        colLoc = glGetUniformLocation(self.program, b"_Color")
        glBindVertexArray(self.renderer.vao)
        glVertexAttrib4f(1, 1.0, 1.0, 1.0, 1.0)
        for x in self.rectangles:
            mat4_trs(self.modelToWorld, x.pos, x.rot, V3(1.0, 1.0, 0.0))
            self.modelToClip = numpy.matmul(cam.worldToClip, self.modelToWorld)
            glUniformMatrix4fv(loc, 1, GL_TRUE, self.modelToClip)
            col4 = V4.fromInt32(x.color)
            glUniform4f(colLoc, col4.x, col4.y, col4.z, col4.w)
            glDrawArrays(GL_TRIANGLES, 0, 6)

class RectangleRenderer2:
    def __init__(self, program, renderer):
        self.program = program
        self.renderer = renderer
        self.rectangles = []
        self.modelToWorld = numpy.empty((4, 4), dtype=numpy.float32)
        self.modelToClip = numpy.empty((4, 4), dtype=numpy.float32)
    def render(self):
        cam = self.renderer.activeCam
        if cam is None:
            return
        glUseProgram(self.program)
        loc = glGetUniformLocation(self.program, b"_ModelToClip")
        outlineLoc = glGetUniformLocation(self.program, b"_Outline")
        colLoc = glGetUniformLocation(self.program, b"_Color")
        glBindVertexArray(self.renderer.vao)
        glVertexAttrib4f(1, 1.0, 1.0, 1.0, 1.0)
        for x in self.rectangles:
            numpy.matmul(cam.worldToClip, x.modelToWorld, self.modelToClip)
            glUniformMatrix4fv(loc, 1, GL_TRUE, ctypes.c_void_p(self.modelToClip.ctypes.data))
            bb = x.border
            glUniform4f(outlineLoc, bb.x, bb.y, bb.z, bb.w)
            col4 = V4.fromInt32(x.color)
            glUniform4f(colLoc, col4.x, col4.y, col4.z, col4.w)
            glDrawArrays(GL_TRIANGLES, 0, 6)

class LineRenderer:
    def __init__(self, program, renderer):
        self.program = program
        self.renderer = renderer
        self.vData = numpy.empty(65536, dtype=numpy.float32)
        self.vDataInt = self.vData.view(dtype=numpy.uint32)
        self.vao = glGenVertexArrays(1)
        glBindVertexArray(self.vao)
        self.vbo = glGenBuffers(1)
        glBindBuffer(GL_ARRAY_BUFFER, self.vbo)
        glBufferData(GL_ARRAY_BUFFER, self.vData.nbytes, self.vData, GL_DYNAMIC_DRAW)
        glEnableVertexAttribArray(0)
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 16, ctypes.c_void_p(0))
        glEnableVertexAttribArray(1)
        glVertexAttribPointer(1, 4, GL_UNSIGNED_BYTE, GL_TRUE, 16, ctypes.c_void_p(12))
        '''self.eData = numpy.array(65536, dtype=numpy.uint16)
        self.ebo = glGenBuffers(1)
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, self.ebo)
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, self.eData.nbytes, self.eData, GL_STATIC_DRAW)'''
        glBindVertexArray(0)
        self.lineStrips = []
        
    def render(self):
        cam = self.renderer.activeCam
        if cam is None:
            return
        glUseProgram(self.program)
        loc = glGetUniformLocation(self.program, b"_ModelToClip")
        glUniformMatrix4fv(loc, 1, GL_TRUE, cam.worldToClip)
        colLoc = glGetUniformLocation(self.program, b"_Color")
        glUniform4f(colLoc, 1.0, 1.0, 1.0, 1.0)
        idx = 0
        segmentIndices = []
        for lineStrip in self.lineStrips:
            # TODO: make sure no overflow
            if len(lineStrip.vertices)*3 + idx >= self.vData.size:
                print("Can't draw some line strips. (Preventing buffer overflow)")
                break
            segmentIndices.append((idx//4, len(lineStrip.vertices), lineStrip.width))
            for vidx in range(len(lineStrip.vertices)):
                vert = lineStrip.vertices[vidx]
                col = lineStrip.colors[vidx]
                self.vData[idx] = vert[0]
                self.vData[idx+1] = vert[1]
                self.vData[idx+2] = vert[2]
                c = col
                # doing this so I can use 32bit vertex attrib with GL_UNSIGNED_BYTE
                self.vDataInt[idx+3] = (c>>24)|(c>>8)&0x0000FF00|(c<<8)&0x00FF0000|(c<<24)
                idx += 4
        glBindVertexArray(self.vao)
        glBufferSubData(GL_ARRAY_BUFFER, 0, idx*self.vData.dtype.itemsize, self.vData)
        for segment in segmentIndices:
            # TODO: stop using glLineWidth or even GL_LINE_STRIP entirely
            # the feature is unreliable and could cause lines to not render
            # or cause driver software rendering in some cases
            glLineWidth(segment[2])
            glDrawArrays(GL_LINE_STRIP, segment[0], segment[1])

class CircleRenderer:
    def __init__(self, program, renderer):
        self.program = program
        self.renderer = renderer
        self.modelToWorld = numpy.empty((4, 4), dtype=numpy.float32)
        self.modelToClip = numpy.empty((4, 4), dtype=numpy.float32)
        self.circles = []
    def render(self):
        cam = self.renderer.activeCam
        if cam is None:
            return
        glUseProgram(self.program)
        loc = glGetUniformLocation(self.program, b"_ModelToClip")
        colLoc = glGetUniformLocation(self.program, b"_Color")
        outlineLoc = glGetUniformLocation(self.program, b"_Outline")
        glBindVertexArray(self.renderer.vao)
        glVertexAttrib4f(1, 1.0, 1.0, 1.0, 1.0)
        for x in self.circles:
            mat4_ts(self.modelToWorld, x.pos, V3(x.radius*2.0, x.radius*2.0, x.radius*2.0))
            numpy.matmul(cam.worldToClip, self.modelToWorld, self.modelToClip)
            glUniformMatrix4fv(loc, 1, GL_TRUE, self.modelToClip)
            col4 = V4.fromInt32(x.color)
            glUniform4f(colLoc, col4.x, col4.y, col4.z, col4.w)
            b = x.outline
            glUniform4f(outlineLoc, b.x, b.y, b.z, b.w)
            glDrawArrays(GL_TRIANGLES, 0, 6)


class CircleRenderer2:
    def __init__(self, program, renderer):
        self.program = program
        self.renderer = renderer
        self.numIndices = 360*3
        vData = numpy.zeros(3*361, dtype=numpy.float32) # sizeof(float)*3*360
        eData = numpy.zeros(self.numIndices, dtype=numpy.uint16)
        idx = 0
        # these are to prevent per frame allocations
        self.modelToWorld = numpy.empty((4, 4), dtype=numpy.float32)
        self.modelToClip = numpy.empty((4, 4), dtype=numpy.float32)
        # populate default mesh
        for i in range(360):
            angle = float(math.radians(float(i)))
            vData[i*3] = float(0.5*math.sin(angle))
            vData[i*3+1] = float(0.5*math.cos(angle))
            # z already 0
            eData[idx+0] = i
            eData[idx+1] = 360 # 360 is center point 0,0,0
            eData[idx+2] = i+1
            idx += 3
        eData[360*3-1] = 0 # last would be 360 otherwise 
        self.vao = glGenVertexArrays(1)
        glBindVertexArray(self.vao)
        self.vbo = glGenBuffers(1)
        glBindBuffer(GL_ARRAY_BUFFER, self.vbo)
        glBufferData(GL_ARRAY_BUFFER, vData.nbytes, vData, GL_STATIC_DRAW)
        glEnableVertexAttribArray(0)
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, ctypes.c_void_p(0))
        self.ebo = glGenBuffers(1)
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, self.ebo)
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, eData.nbytes, eData, GL_STATIC_DRAW) 
        glBindVertexArray(0)
        self.circles = []

    def render(self):
        cam = self.renderer.activeCam
        if cam is None:
            return
        glUseProgram(self.program)
        loc = glGetUniformLocation(self.program, b"_ModelToClip")
        colLoc = glGetUniformLocation(self.program, b"_Color")
        glBindVertexArray(self.vao)
        glVertexAttrib4f(1, 1.0, 1.0, 1.0, 1.0)
        for x in self.circles:
            mat4_translate(self.modelToWorld, x.pos)
            numpy.matmul(cam.worldToClip, self.modelToWorld, self.modelToClip)
            glUniformMatrix4fv(loc, 1, GL_TRUE, self.modelToClip)
            col4 = V4.fromInt32(x.color)
            glUniform4f(colLoc, col4.x, col4.y, col4.z, col4.w)
            glDrawElements(GL_TRIANGLES, self.numIndices, GL_UNSIGNED_SHORT, ctypes.c_void_p(0))


