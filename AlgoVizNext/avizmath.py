import numpy
import math
import copy
import sys
import struct

# something is wrong with these, yellow to hsl -> rgb doesn't look yellow
def rgbToHCV(rgb): # hue/chroma/value
    p = V4(rgb.z, rgb.y, -1.0, 2.0/3.0) if rgb.y < rgb.z else V4(rgb.y, rgb.z, 0.0, -1.0/3.0)
    q = V4(p.x, p.y, p.w, rgb.x) if rgb.x < p.x else V4(rgb.x, p.y, p.z, p.x)
    c = q.x - min(q.w, q.y)
    return V3(abs((q.w - q.y) / (6.0 * c + 0.000000000000000001) + q.z), c, q.x)
def rgbToHSL(rgb):
    hcv = rgbToHCV(rgb)
    l = hcv.z - hcv.y * 0.5
    s = hcv.y / (1.0 - abs(l * 2.0 - 1.0) + 0.000000000000001)
    return V3(hcv.x, s, l)
def hueToRGB(h):
    r = abs(h*6.0 - 3.0) - 1.0
    g = 2.0 - abs(h * 6.0 - 2.0)
    b = 2.0 - abs(h * 6.0 - 4.0)
    ret = V3(r, g, b)
    ret.saturate()
    return ret
def hslToRGB(hsl):
    rgb = hueToRGB(hsl.x)
    c = (1.0 - abs(2.0 * hsl.z - 1.0)) * hsl.y
    return V3((rgb.x - 0.5)*c + hsl.z, (rgb.y - 0.5)*c + hsl.z, (rgb.z - 0.5)*c + hsl.z)
def rgbToInt32(rgb):
    data = [255, int(rgb.z*255), int(rgb.y*255), int(rgb.x*255)]
    values = struct.unpack("I", bytearray(data))
    return values[0]

class V3:
    def __init__(self, x, y, z):
        self.x = x
        self.y = y
        self.z = z
    def normalize(self):
        vlen = math.sqrt(self.x*self.x+self.y*self.y+self.z*self.z)
        self.x /= vlen
        self.y /= vlen
        self.z /= vlen
    def saturate(self):
        self.x  = max(self.x, 0.0)
        self.y  = max(self.y, 0.0)
        self.z  = max(self.z, 0.0)
        vlen = math.sqrt(self.x*self.x+self.y*self.y+self.z*self.z)
        self.x /= vlen
        self.y /= vlen
        self.z /= vlen
    @staticmethod
    def lerp(v1, v2, t):
        return V3(v1.x + (v2.x-v1.x)*t, v1.y + (v2.y-v1.y)*t, v1.z + (v2.z-v1.z)*t)
    def __iadd__(self, other):
        self.x += other.x
        self.y += other.y
        self.z += other.z
        return self
    def __isub__(self, other):
        self.x -= other.x
        self.y -= other.y
        self.z -= other.z
        return self
class V4:
    def __init__(self, x, y, z, w):
        self.x = x
        self.y = y
        self.z = z
        self.w = w
    @classmethod
    def fromInt32(cls, val):
        x = float((val>>24)&0xFF)/255.0
        y = float((val>>16)&0xFF)/255.0
        z = float((val>>8)&0xFF)/255.0
        w = float((val)&0xFF)/255.0
        return cls(x, y, z, w)


class Quat:
    def __init__(self, w, x, y, z):
        self.w = w
        self.x = x
        self.y = y
        self.z = z
    @classmethod
    def angleAxis(cls, angleDeg, axis):
        halfARad = math.radians(angleDeg)*0.5;
        sinHAR = math.sin(halfARad);
        return cls(math.cos(halfARad), axis.x*sinHAR, axis.y*sinHAR, axis.z*sinHAR)
    @classmethod
    def z2d(cls, angle): # 2d rotation perpendicular to z axis
        hrad = math.radians(angle)*0.5
        return cls(math.cos(hrad), 0.0, 0.0, math.sin(hrad)) 

V3.id = V3(0.0, 0.0, 0.0)
V3.one = V3(1.0, 1.0, 1.0)
V4.id = V4(0.0, 0.0, 0.0, 0.0)
V4.one = V4(1.0, 1.0, 1.0, 1.0)
Quat.id = Quat(1.0, 0.0, 0.0, 0.0)

# slow as fuck :-/
def mat4_mul(m1, m2, dst):
    dst[0,0] = m1[0,0] * m2[0,0] + m1[0,1] * m2[1,0] + m1[0,2] * m2[2,0] + m1[0,3] * m2[3,0];
    dst[1,0] = m1[1,0] * m2[0,0] + m1[1,1] * m2[1,0] + m1[1,2] * m2[2,0] + m1[1,3] * m2[3,0];
    dst[2,0] = m1[2,0] * m2[0,0] + m1[2,1] * m2[1,0] + m1[2,2] * m2[2,0] + m1[2,3] * m2[3,0];
    dst[3,0] = m1[3,0] * m2[0,0] + m1[3,1] * m2[1,0] + m1[3,2] * m2[2,0] + m1[3,3] * m2[3,0];
    dst[0,1] = m1[0,0] * m2[0,1] + m1[0,1] * m2[1,1] + m1[0,2] * m2[2,1] + m1[0,3] * m2[3,1];
    dst[1,1] = m1[1,0] * m2[0,1] + m1[1,1] * m2[1,1] + m1[1,2] * m2[2,1] + m1[1,3] * m2[3,1];
    dst[2,1] = m1[2,0] * m2[0,1] + m1[2,1] * m2[1,1] + m1[2,2] * m2[2,1] + m1[2,3] * m2[3,1];
    dst[3,1] = m1[3,0] * m2[0,1] + m1[3,1] * m2[1,1] + m1[3,2] * m2[2,1] + m1[3,3] * m2[3,1];
    dst[0,2] = m1[0,0] * m2[0,2] + m1[0,1] * m2[1,2] + m1[0,2] * m2[2,2] + m1[0,3] * m2[3,2];
    dst[1,2] = m1[1,0] * m2[0,2] + m1[1,1] * m2[1,2] + m1[1,2] * m2[2,2] + m1[1,3] * m2[3,2];
    dst[2,2] = m1[2,0] * m2[0,2] + m1[2,1] * m2[1,2] + m1[2,2] * m2[2,2] + m1[2,3] * m2[3,2];
    dst[3,2] = m1[3,0] * m2[0,2] + m1[3,1] * m2[1,2] + m1[3,2] * m2[2,2] + m1[3,3] * m2[3,2];
    dst[0,3] = m1[0,0] * m2[0,3] + m1[0,1] * m2[1,3] + m1[0,2] * m2[2,3] + m1[0,3] * m2[3,3];
    dst[1,3] = m1[1,0] * m2[0,3] + m1[1,1] * m2[1,3] + m1[1,2] * m2[2,3] + m1[1,3] * m2[3,3];
    dst[2,3] = m1[2,0] * m2[0,3] + m1[2,1] * m2[1,3] + m1[2,2] * m2[2,3] + m1[2,3] * m2[3,3];
    dst[3,3] = m1[3,0] * m2[0,3] + m1[3,1] * m2[1,3] + m1[3,2] * m2[2,3] + m1[3,3] * m2[3,3];

def mat4_perspective(mat, fov, aspect, zNear, zFar):
    # OPTIMIZE: wasteful?
    mat.fill(0)
    tanHalfFOV = math.tan(math.radians(fov*0.5))
    depth = zNear-zFar
    mat[0,0] = 1.0 / (tanHalfFOV * aspect)
    mat[1,1] = 1.0 / (tanHalfFOV)
    mat[2,2] = (-zNear - zFar) / depth
    mat[3,2] = 1.0
    mat[2,3] = 2.0 * zFar * zNear / depth

def mat4_translate(mat, offset):
    mat.fill(0)
    mat[0, 0] = 1.0
    mat[1, 1] = 1.0
    mat[2, 2] = 1.0
    mat[3, 3] = 1.0
    mat[0, 3] = offset.x
    mat[1, 3] = offset.y
    mat[2, 3] = offset.z

def mat4_rotation(mat, r):
    mat[0,0] = 1.0 - 2.0*r.y*r.y - 2.0*r.z*r.z
    mat[1,0] = 2.0*r.x*r.y + 2.0*r.z*r.w
    mat[2,0] = 2.0*r.x*r.z - 2.0*r.y*r.w
    mat[3,0] = 0.0

    mat[0,1] = 2.0*r.x*r.y - 2.0*r.z*r.w
    mat[1,1] = 1.0 - 2.0*r.x*r.x - 2.0*r.z*r.z
    mat[2,1] = 2.0*r.y*r.z + 2.0*r.x*r.w
    mat[3,1] = 0.0
    
    mat[0,2] = 2.0*r.x*r.z + 2.0*r.y*r.w
    mat[1,2] = 2.0*r.y*r.z - 2.0*r.x*r.w
    mat[2,2] = 1.0 - 2.0*r.x*r.x - 2.0*r.y*r.y
    mat[3,2] = 0.0

    mat[0,3] = 0.0
    mat[1,3] = 0.0
    mat[2,3] = 0.0
    mat[3,3] = 1.0

def mat4_trs(mat, t, r, s):
    mat[0,0] = (1.0-2.0*(r.y*r.y+r.z*r.z))*s.x
    mat[0,1] = (r.x*r.y-r.z*r.w)*s.y*2.0
    mat[0,2] = (r.x*r.z+r.y*r.w)*s.z*2.0
    mat[0,3] = t.x
    mat[1,0] = (r.x*r.y+r.z*r.w)*s.x*2.0
    mat[1,1] = (1.0-2.0*(r.x*r.x+r.z*r.z))*s.y
    mat[1,2] = (r.y*r.z-r.x*r.w)*s.z*2.0
    mat[1,3] = t.y
    mat[2,0] = (r.x*r.z-r.y*r.w)*s.z*2.0
    mat[2,1] = (r.y*r.z+r.x*r.w)*s.y*2.0
    mat[2,2] = (1.0-2.0*(r.x*r.x+r.y*r.y))*s.z
    mat[2,3] = t.z
    mat[3,0] = 0.0
    mat[3,1] = 0.0
    mat[3,2] = 0.0
    mat[3,3] = 1.0
    return mat

def mat4_ts(mat, t, s):
    mat[0,0] = s.x
    mat[1,0] = 0.0
    mat[2,0] = 0.0
    mat[3,0] = 0.0
    mat[0,1] = 0.0
    mat[1,1] = s.y
    mat[2,1] = 0.0
    mat[3,1] = 0.0
    mat[0,2] = 0.0
    mat[1,2] = 0.0
    mat[2,2] = s.z
    mat[3,2] = 0.0
    mat[0,3] = t.x
    mat[1,3] = t.y
    mat[2,3] = t.z
    mat[3,3] = 1.0
    return mat

def mat4_rt(rotation, translation):
    # TODO: expand and optimize
    translate = numpy.zeros((4,4), dtype=numpy.float32)
    rotate = numpy.zeros((4,4), dtype=numpy.float32)
    mat4_translate(translate, translation)
    mat4_rotation(rotate, rotation)
    return numpy.matmul(rotate, translate)


