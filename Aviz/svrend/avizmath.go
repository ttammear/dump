package svrend

import "math"

type IV2 struct {
    X, Y int32
}

type V2 struct {
    X, Y float32
}

type V3 struct {
    X, Y, Z float32
}

type IV4 struct {
    X, Y, Z, W int32
}

type V4 struct {
    X, Y, Z, W float32
}

type Quat struct {
    W, X, Y, Z float32
}

type Mat4 struct {
    M [4][4]float32
}

func max(x, y float32) float32 {
    if x > y {
        return x
    }
    return y
}
func min(x, y float32) float32 {
    if x < y {
        return x
    }
    return y
}

func (v *V3) Normalize() {
    vlen := float32(math.Sqrt(float64(v.X*v.X + v.Y*v.Y + v.Z*v.Z)))
    v.X /= vlen
    v.Y /= vlen
    v.Z /= vlen
}

func (v *V3) Saturate() {
    v.X = max(v.X, 0.0)
    v.Y = max(v.Y, 0.0)
    v.Z = max(v.Z, 0.0)
    vlen := float32(math.Sqrt(float64(v.X*v.X + v.Y*v.Y + v.Z*v.Z)))
    v.X /= vlen
    v.Y /= vlen
    v.Z /= vlen
}

func (v *V3) Added(v2 V3) V3{
    return V3{v.X+v2.X, v.Y+v2.Y, v.Z+v2.Z};
}

func V3Add(v, v2 V3) V3 {
    return V3{v.X+v2.X, v.Y+v2.Y, v.Z+v2.Z};
}

func V3Lerp(v1, v2 V3, t float32) V3 {
    return V3{v1.X + (v2.X-v1.X)*t, v1.Y + (v2.Y-v1.Y)*t, v1.Z + (v2.Z-v1.Z)*t}
}

func V4FromInt32(val uint32) V4 {
    return V4{float32((val>>24)&0xFF)/255.0, float32((val>>16)&0xFF)/255.0, float32((val>>8)&0xFF)/255.0, float32((val)&0xFF)/255.0}
}

func angleAxis(angleDeg float32, axis V3) Quat {
    halfARad := (angleDeg/360.0)*math.Pi
    sinHAR := float32(math.Sin(float64(halfARad)))
    return Quat{float32(math.Cos(float64(halfARad))), axis.X*sinHAR, axis.Y*sinHAR, axis.Z*sinHAR}
}

func Z2d(angle float32) Quat {
    hrad := (angle/360)*math.Pi
    return Quat{float32(math.Cos(float64(hrad))), 0.0, 0.0, float32(math.Sin(float64(hrad)))}
}

func Y2d(angle float32) Quat {
    hrad := (angle/360)*math.Pi
    return Quat{float32(math.Cos(float64(hrad))), 0.0, float32(math.Sin(float64(hrad))), 0.0}
}

func X2d(angle float32) Quat {
    hrad := (angle/360)*math.Pi
    return Quat{float32(math.Cos(float64(hrad))), float32(math.Sin(float64(hrad))), 0.0, 0.0}
}

func Quat_mul(l, r Quat) Quat {
    var res Quat
	res.W = l.W * r.W - l.X * r.X - l.Y * r.Y - l.Z * r.Z;
	res.X = l.W * r.X + l.X * r.W + l.Y * r.Z - l.Z * r.Y;
	res.Y = l.W * r.Y - l.X * r.Z + l.Y * r.W + l.Z * r.X;
	res.Z = l.W * r.Z + l.X * r.Y - l.Y * r.X + l.Z * r.W;
    return res
}

func V4_transform(m *Mat4, v V4) V4 {
    var res V4
    res.X = v.X * m.M[0][0] + v.Y * m.M[1][0] + v.Z * m.M[2][0] + v.W * m.M[3][0];
    res.Y = v.X * m.M[0][1] + v.Y * m.M[1][1] + v.Z * m.M[2][1] + v.W * m.M[3][1];
    res.Z = v.X * m.M[0][2] + v.Y * m.M[1][2] + v.Z * m.M[2][2] + v.W * m.M[3][2];
    res.W = v.X * m.M[0][3] + v.Y * m.M[1][3] + v.Z * m.M[2][3] + v.W * m.M[3][3];
    return res
}

func V4_length(v V4) float32 {
    return float32(math.Sqrt((float64)(v.X*v.X+v.Y*v.Y+v.Z*v.Z+v.W*v.W)));
}

func mat4_mul(dst, m1, m2 *Mat4) {
    for row := 0; row < 4; row++ {
        for col := 0; col < 4; col++ {
            dst.M[row][col] = 0.0
            for n := 0; n < 4; n++ {
                dst.M[row][col] += m1.M[n][col]*m2.M[row][n]
            }
        }
    }
}

func mat4_perspective(m *Mat4, fov, aspect, zNear, zFar float32) {
    tanHalfFOV := float32(math.Tan((float64(fov)/360.0)*math.Pi))
    depth := zNear-zFar
    for row := 0; row < 4; row++ {
        for col := 0; col < 4; col++ {
            m.M[row][col] = 0.0
        }
    }
    m.M[0][0] = 1.0 / (tanHalfFOV * aspect)
    m.M[1][1] = 1.0 / (tanHalfFOV)
    m.M[2][2] = (-zNear - zFar) / depth
    m.M[2][3] = 1.0
    m.M[3][2] = (2.0 * zFar * zNear) / depth
}

func mat4_ortho(m *Mat4, l, r, t, b, f, n float32) {
    m.M[0][0] = 2.0 / (r-l)
    m.M[0][1] = 0.0
    m.M[0][2] = 0.0
    m.M[0][3] = 0.0
    m.M[1][0] = 0.0
    m.M[1][1] = 2.0 / (b-t)
    m.M[1][2] = 0.0
    m.M[1][3] = 0.0
    m.M[2][0] = 0.0
    m.M[2][1] = 0.0
    m.M[2][2] = -2.0 / (f-n)
    m.M[2][3] = 0.0
    m.M[3][0] = 0.0
    m.M[3][1] = 0.0
    m.M[3][2] = 0.0
    m.M[3][3] = 1.0
}

func mat4_translate(m *Mat4, offset V3) {
    m.M[0][0] = 1.0
    m.M[0][1] = 0.0
    m.M[0][2] = 0.0
    m.M[0][3] = 0.0
    m.M[1][0] = 0.0
    m.M[1][1] = 1.0
    m.M[1][2] = 0.0
    m.M[1][3] = 0.0
    m.M[2][0] = 0.0
    m.M[2][1] = 0.0
    m.M[2][2] = 1.0
    m.M[2][3] = 0.0
    m.M[3][0] = offset.X
    m.M[3][1] = offset.Y
    m.M[3][2] = offset.Z
    m.M[3][3] = 1.0
}

func mat4_rotation(m *Mat4, r Quat) {
    m.M[0][0] = 1.0 - 2.0*r.Y*r.Y - 2.0*r.Z*r.Z
    m.M[0][1] = 2.0*r.X*r.Y + 2.0*r.Z*r.W
    m.M[0][2] = 2.0*r.X*r.Z - 2.0*r.Y*r.W
    m.M[0][3] = 0.0

    m.M[1][0] = 2.0*r.X*r.Y - 2.0*r.Z*r.W
    m.M[1][1] = 1.0 - 2.0*r.X*r.X - 2.0*r.Z*r.Z
    m.M[1][2] = 2.0*r.Y*r.Z + 2.0*r.X*r.W
    m.M[1][3] = 0.0
    
    m.M[2][0] = 2.0*r.X*r.Z + 2.0*r.Y*r.W
    m.M[2][1] = 2.0*r.Y*r.Z - 2.0*r.X*r.W
    m.M[2][2] = 1.0 - 2.0*r.X*r.X - 2.0*r.Y*r.Y
    m.M[2][3] = 0.0

    m.M[3][0] = 0.0
    m.M[3][1] = 0.0
    m.M[3][2] = 0.0
    m.M[3][3] = 1.0
}

func mat4_trs(m *Mat4, t V3, r Quat, s V3) {
    m.M[0][0] = (1.0-2.0*(r.Y*r.Y+r.Z*r.Z))*s.X
    m.M[1][0] = (r.X*r.Y-r.Z*r.W)*s.Y*2.0
    m.M[2][0] = (r.X*r.Z+r.Y*r.W)*s.Z*2.0
    m.M[3][0] = t.X
    m.M[0][1] = (r.X*r.Y+r.Z*r.W)*s.X*2.0
    m.M[1][1] = (1.0-2.0*(r.X*r.X+r.Z*r.Z))*s.Y
    m.M[2][1] = (r.Y*r.Z-r.X*r.W)*s.Z*2.0
    m.M[3][1] = t.Y
    m.M[0][2] = (r.X*r.Z-r.Y*r.W)*s.Z*2.0
    m.M[1][2] = (r.Y*r.Z+r.X*r.W)*s.Y*2.0
    m.M[2][2] = (1.0-2.0*(r.X*r.X+r.Y*r.Y))*s.Z
    m.M[3][2] = t.Z
    m.M[0][3] = 0.0
    m.M[1][3] = 0.0
    m.M[2][3] = 0.0
    m.M[3][3] = 1.0
}

func mat4_ts(m *Mat4, t V3, s V3) {
    m.M[0][0] = s.X
    m.M[0][1] = 0.0
    m.M[0][2] = 0.0
    m.M[0][3] = 0.0
    m.M[1][0] = 0.0
    m.M[1][1] = s.Y
    m.M[1][2] = 0.0
    m.M[1][3] = 0.0
    m.M[2][0] = 0.0
    m.M[2][1] = 0.0
    m.M[2][2] = s.Z
    m.M[2][3] = 0.0
    m.M[3][0] = t.X
    m.M[3][1] = t.Y
    m.M[3][2] = t.Z
    m.M[3][3] = 1.0
}

func mat4_rt(m *Mat4, r Quat, t V3) {
    var tm, rm Mat4
    mat4_translate(&tm, t)
    mat4_rotation(&rm, r)
    mat4_mul(m, &rm, &tm)
}
