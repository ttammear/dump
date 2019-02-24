#pragma once

#include <math.h>
#include "libs/tt_types.h"

#define DEG2RAD_F 0.0174532925f

typedef struct V2
{
    r32 x,y;
} V2;

typedef struct V3
{
    r32 x, y, z;
} V3;

typedef struct V4
{
    r32 x, y, z, w;
} V4;

typedef struct Quat
{
    r32 w, x, y, z;
} Quat;

typedef struct Mat4
{
    union {
        struct
        {
            r32 m[16];
        } ;

        struct
        {
            r32 cr[4][4];
        };

        struct
        {
            r32 m11, m21, m31, m41;
            r32 m12, m22, m32, m42;
            r32 m13, m23, m33, m43;
            r32 m14, m24, m34, m44;
        };
    };
}Mat4;

typedef struct Mat3
{
    union {
        struct
        {
            float m[9];
        };
        struct 
        {
            float cr[3][3];
        };
        struct {
            float m11, m21, m31;
            float m12, m22, m32;
            float m13, m23, m33;
        };
    };
} Mat3;

#ifdef AIKE_X86

typedef struct Mat4_sse2
{
    union {
        struct {
            __m128 m11, m21, m31, m41;
            __m128 m12, m22, m32, m42;
            __m128 m13, m23, m33, m43;
            __m128 m14, m24, m34, m44;
        } ;
        struct
        {
            __m128 mm[4][4];
        };
        struct
        {
            __m128 m[16];
        };
        struct
        {
            float f[16][4];
        };
    };
}Mat4_sse2;

void mat4_mul_sse2(struct Mat4_sse2 *m, struct Mat4_sse2 *l, struct Mat4_sse2 *r)
{
    for (int row = 0; row < 4; row++)
	{
		for (int col = 0; col < 4; col++)
		{
            m->mm[row][col] = _mm_add_ps(_mm_mul_ps(l->mm[0][col], r->mm[row][0]), 
                    _mm_add_ps(_mm_mul_ps(l->mm[1][col], r->mm[row][1]), 
                    _mm_add_ps(_mm_mul_ps(l->mm[2][col], r->mm[row][2]), 
                    _mm_mul_ps(l->mm[3][col], r->mm[row][3]))));
		}
	}
}

static inline void mat4_load_sse2(struct Mat4_sse2 *dst, struct Mat4* m1, struct Mat4 *m2, struct Mat4 *m3, struct Mat4 *m4)
{
    for(int i = 0; i < 16; i++)
    {
        _Alignas(16) float data[4] = {m1->m[i], m2->m[i], m3->m[i], m4->m[i]};
        dst->m[i] = _mm_load_ps(data);
    }
}

static inline void mat4_extract_sse2(struct Mat4 *dst, struct Mat4_sse2 *src, u32 idx)
{
    _Alignas(16) float data[4];
    for(int i = 0; i < 16; i++)
    {
        _mm_store_ps(data, src->m[i]);
        dst->m[i] = data[idx];
    }
}

static inline void mat4_extract_all_sse2(struct Mat4 *restrict dst, struct Mat4_sse2 *restrict src)
{
    _Alignas(16) float data[4];
    for(int i = 0; i < 16; i++)
    {
        _mm_store_ps(data, src->m[i]);
        dst[0].m[i] = data[0];
        dst[1].m[i] = data[1];
        dst[2].m[i] = data[2];
        dst[3].m[i] = data[3];
    }
}

#else

typedef struct Mat4_sse2
{
    union {
        struct {
            struct Mat4 mat[4];
        } Mat4_sse2;
        struct
        {
            float f[16][4];
        };
    };
};

void mat4_mul_sse2(struct Mat4_sse2 *m, struct Mat4_sse2 *l, struct Mat4_sse2 *r)
{
    assert(false);
}

static inline void mat4_load_sse2(struct Mat4_sse2 *dst, struct Mat4* m1, struct Mat4 *m2, struct Mat4 *m3, struct Mat4 *m4)
{
    assert(false);
}

static inline void mat4_extract_sse2(struct Mat4 *dst, struct Mat4_sse2 *src, u32 idx)
{
    assert(false);
}

static inline void mat4_extract_all_sse2(struct Mat4 *restrict dst, struct Mat4_sse2 *restrict src)
{
    assert(false);
}


#endif

static inline struct V2 make_v2(r32 x, r32 y)
{
    struct V2 ret;
    ret.x = x;
    ret.y = y;
    return ret;
}

static inline struct V3 make_v3(r32 x, r32 y, r32 z)
{
    struct V3 ret = {x, y, z};
    return ret;
}

static inline struct V4 make_v4(r32 x, r32 y, r32 z, r32 w)
{
    struct V4 ret = {x, y, z, w};
    return ret;
}

static inline struct Quat make_quat(r32 w, r32 x, r32 y, r32 z)
{
    struct Quat ret = {w, x, y, z};
    return ret;
}

static inline V2 v2_add(V2 *restrict res, V2 lhs, V2 rhs)
{
    res->x = lhs.x + rhs.x;
    res->y = lhs.y + rhs.y;
    return *res;
}

static inline V2 v2_sub(V2 *restrict res, V2 lhs, V2 rhs)
{
    res->x = lhs.x - rhs.x;
    res->y = lhs.y - rhs.y;
    return *res;
}

static inline void v2_normalize(V2 *v)
{
    float len = sqrtf(v->x*v->x + v->y*v->y);
    v->x = v->x / len;
    v->y = v->y / len;
}

static inline float v2_dot(V2 l, V2 r)
{
    return l.x*r.x + l.y*r.y;
}

static inline float v2_len(V2 v)
{
    return sqrtf(v.x*v.x + v.y*v.y);
}

static inline V3 v3_add(V3 *restrict res, V3 lhs, V3 rhs)
{
    res->x = lhs.x + rhs.x;
    res->y = lhs.y + rhs.y;
    res->z = lhs.z + rhs.z;
    return *res;
}

static inline V3 v3_scale(V3 v, float scale) {
    return make_v3(v.x * scale, v.y * scale, v.z * scale);
}

static inline V3 v3_sub(V3 *restrict res, V3 lhs, V3 rhs)
{
    res->x = lhs.x - rhs.x;
    res->y = lhs.y - rhs.y;
    res->z = lhs.z - rhs.z;
    return *res;
}

static inline void v3_normalize(V3 *v)
{
    float len = sqrtf(v->x*v->x + v->y*v->y + v->z*v->z);
    v->x /= len;
    v->y /= len;
    v->z /= len;
}

static inline bool v3_hasnan(V3 v)
{
    return isnan(v.x) || isnan(v.y) || isnan(v.z);
}

static inline V3 v3_lerp(V3 a, V3 b, float t)
{
    return (V3){a.x + t*(b.x-a.x), a.y + t*(b.y-a.y), a.z + t*(b.z-a.z)};
}

static inline void quat_identity(struct Quat *q)
{
    q->w = 1.0f;
    q->x = 0.0f;
    q->y = 0.0f;
    q->z = 0.0f;
}

static inline void mat4_v4_mul(V4 *restrict res, struct Mat4 *restrict m, V4 v)
{
    res->x = v.x * m->m11 + v.y * m->m12 + v.z * m->m13 + v.w * m->m14;
    res->y = v.x * m->m21 + v.y * m->m22 + v.z * m->m23 + v.w * m->m24;
    res->z = v.x * m->m31 + v.y * m->m32 + v.z * m->m33 + v.w * m->m34;
    res->w = v.x * m->m41 + v.y * m->m42 + v.z * m->m43 + v.w * m->m44;
}

static inline void quat_angle_axis(struct Quat *q, r32 angleDeg, struct V3 axis)
{
    r32 halfAngleRad = (DEG2RAD_F*angleDeg) / 2.0f;
    r32 sinHAR = sinf(halfAngleRad);
    q->w = cosf(halfAngleRad);
    // TODO: axis should be normalized?
    q->x = axis.x * sinHAR;
    q->y = axis.y * sinHAR;
    q->z = axis.z * sinHAR;
}

static inline void quat_euler(struct Quat *q, struct V3 euler)
{
	float cy = cosf(euler.x * 0.5f);
	float sy = sinf(euler.x * 0.5f);
	float cr = cosf(euler.y * 0.5f);
	float sr = sinf(euler.y * 0.5f);
	float cp = cosf(euler.z * 0.5f);
	float sp = sinf(euler.z * 0.5f);
	q->w = cy * cr * cp + sy * sr * sp;
	q->x = cy * sr * cp - sy * cr * sp;
	q->y = cy * cr * sp + sy * sr * cp;
	q->z = sy * cr * cp - cy * sr * sp;
}

static inline void quat_euler_deg(struct Quat *restrict q, struct V3 euler)
{
	float cy = cosf(euler.x * 0.5f * DEG2RAD_F);
	float sy = sinf(euler.x * 0.5f * DEG2RAD_F);
	float cr = cosf(euler.y * 0.5f * DEG2RAD_F);
	float sr = sinf(euler.y * 0.5f * DEG2RAD_F);
	float cp = cosf(euler.z * 0.5f * DEG2RAD_F);
	float sp = sinf(euler.z * 0.5f * DEG2RAD_F);
	q->w = cy * cr * cp + sy * sr * sp;
	q->x = cy * sr * cp - sy * cr * sp;
	q->y = cy * cr * sp + sy * sr * cp;
	q->z = sy * cr * cp - cy * sr * sp;
}

static inline void quat_mul(Quat *restrict res, Quat l, Quat r)
{
	res->w = l.w * r.w - l.x * r.x - l.y * r.y - l.z * r.z;
	res->x = l.w * r.x + l.x * r.w + l.y * r.z - l.z * r.y;
	res->y = l.w * r.y - l.x * r.z + l.y * r.w + l.z * r.x;
	res->z = l.w * r.z + l.x * r.y - l.y * r.x + l.z * r.w;
}

static inline struct V3 normalize_degrees(struct V3 degRot)
{
    degRot.x = degRot.x < 0.0f ? 360.0f + degRot.x : degRot.x;
    degRot.y = degRot.y < 0.0f ? 360.0f + degRot.y : degRot.y;
    degRot.z = degRot.z < 0.0f ? 360.0f + degRot.z : degRot.z;
    return (struct V3) {fmodf(degRot.x, 360.0f), fmodf(degRot.y, 360.0f), fmodf(degRot.z, 360.0f)};
}

static inline void mat4_perspective(struct Mat4 *m, r32 fov, r32 aspect, r32 zNear, r32 zFar)
{
    const r32 tanHalfFOV = tanf(fov*DEG2RAD_F*0.5f);
    r32 depth = zNear - zFar;
    m->m11 = 1.0f / (tanHalfFOV * aspect);
    m->m21 = 0.0f;
    m->m31 = 0.0f;
    m->m41 = 0.0f;

    m->m12 = 0.0f;
    m->m22 = 1.0f / tanHalfFOV;
    m->m32 = 0.0f;
    m->m42 = 0.0f;

    m->m13 = 0.0f;
    m->m23 = 0.0f;
    m->m33 = (-zNear - zFar) / depth;
    m->m43 = 1.0f;

    m->m14 = 0.0f;
    m->m24 = 0.0f;
    m->m34 = 2.0f * zFar * zNear / depth;
    m->m44 = 0.0f;
}

static inline void inverse_perspective(Mat4 *dest, Mat4 *perspectiveMat)
{
    float a = perspectiveMat->m[0];
    float b = perspectiveMat->m[5];
    float c = perspectiveMat->m[10];
    float d = perspectiveMat->m[14];
    float e = perspectiveMat->m[11];

    for(int i = 0; i < 16; i++)
        dest->m[i]  = 0.0f;

    dest->m[0]  = 1.0f / a;
    dest->m[5]  = 1.0f / b;
    dest->m[11] = 1.0f / d;
    dest->m[14] = 1.0f / e;
    dest->m[15] = -c / (d * e);
}

static inline void mat4_ortho(struct Mat4 *m, r32 l, r32 r, r32 b, r32 t, r32 zn, r32 zf)
{
    m->m11 = 2.0f / (r - l);
    m->m21 = 0.0f;
    m->m31 = 0.0f;
    m->m41 = 0.0f;

    m->m12 = 0.0f;
    m->m22 = 2.0f / (t - b);
    m->m32 = 0.0f;
    m->m42 = 0.0f;

    m->m13 = 0.0f;
    m->m23 = 0.0f;
    m->m33 = -2.0f/(zf-zn);
    m->m43 = 0.0f;

    m->m14 = -(r+l)/(r-l);
    m->m24 = -(t+b)/(t-b);
    m->m34 = -(zf+zn)/(zf-zn);
    m->m44 = 1.0f;
}

static inline void mat4_rotation(struct Mat4 *restrict m, struct Quat *restrict q)
{
    m->m11 = 1.0f - 2.0f*q->y*q->y - 2.0f*q->z*q->z;
	m->m21 = 2.0f*q->x*q->y + 2.0f*q->z*q->w;
	m->m31 = 2.0f*q->x*q->z - 2.0f*q->y*q->w;
	m->m41 = 0.0f;

	m->m12 = 2.0f*q->x*q->y - 2.0f*q->z*q->w;
	m->m22 = 1.0f - 2.0f*q->x*q->x - 2.0f*q->z*q->z;
	m->m32 = 2.0f*q->y*q->z + 2.0f*q->x*q->w;
	m->m42 = 0.0f;

	m->m13 = 2.0f*q->x*q->z + 2.0f*q->y*q->w;
	m->m23 = 2.0f*q->y*q->z - 2.0f*q->x*q->w;
	m->m33 = 1.0f - 2.0f*q->x*q->x - 2.0f*q->y*q->y;
	m->m43 = 0.0f;

	m->m14 = 0.0f;
	m->m24 = 0.0f;
	m->m34 = 0.0f;
	m->m44 = 1.0f;
}

static inline void mat4_identity(struct Mat4 *m)
{
    m->m11 = 1.0f; m->m21 = 0.0f; m->m31 = 0.0f; m->m41 = 0.0f;
    m->m12 = 0.0f; m->m22 = 1.0f; m->m32 = 0.0f; m->m42 = 0.0f;
    m->m13 = 0.0f; m->m23 = 0.0f; m->m33 = 1.0f; m->m43 = 0.0f;
    m->m14 = 0.0f; m->m24 = 0.0f; m->m34 = 0.0f; m->m44 = 1.0f;
}

static inline void mat4_mul(struct Mat4 *restrict m, struct Mat4 *restrict l, struct Mat4 *restrict r)
{
/*	
    memset(m, 0, sizeof(struct Mat4));
    for (int row = 0; row < 4; row++)
	{
		for (int col = 0; col < 4; col++)
		{
			for (int n = 0; n < 4; n++)
			{
				m->cr[row][col] += l->cr[n][col] * r->cr[row][n];
			}
		}
	}
*/
    // compiler just fails to unroll it, this is 2x faster
    m->m11 = l->m11 * r->m11 + l->m12 * r->m21 + l->m13 * r->m31 + l->m14 * r->m41;
    m->m21 = l->m21 * r->m11 + l->m22 * r->m21 + l->m23 * r->m31 + l->m24 * r->m41;
    m->m31 = l->m31 * r->m11 + l->m32 * r->m21 + l->m33 * r->m31 + l->m34 * r->m41;
    m->m41 = l->m41 * r->m11 + l->m42 * r->m21 + l->m43 * r->m31 + l->m44 * r->m41;
    m->m12 = l->m11 * r->m12 + l->m12 * r->m22 + l->m13 * r->m32 + l->m14 * r->m42;
    m->m22 = l->m21 * r->m12 + l->m22 * r->m22 + l->m23 * r->m32 + l->m24 * r->m42;
    m->m32 = l->m31 * r->m12 + l->m32 * r->m22 + l->m33 * r->m32 + l->m34 * r->m42;
    m->m42 = l->m41 * r->m12 + l->m42 * r->m22 + l->m43 * r->m32 + l->m44 * r->m42;
    m->m13 = l->m11 * r->m13 + l->m12 * r->m23 + l->m13 * r->m33 + l->m14 * r->m43;
    m->m23 = l->m21 * r->m13 + l->m22 * r->m23 + l->m23 * r->m33 + l->m24 * r->m43;
    m->m33 = l->m31 * r->m13 + l->m32 * r->m23 + l->m33 * r->m33 + l->m34 * r->m43;
    m->m43 = l->m41 * r->m13 + l->m42 * r->m23 + l->m43 * r->m33 + l->m44 * r->m43;
    m->m14 = l->m11 * r->m14 + l->m12 * r->m24 + l->m13 * r->m34 + l->m14 * r->m44;
    m->m24 = l->m21 * r->m14 + l->m22 * r->m24 + l->m23 * r->m34 + l->m24 * r->m44;
    m->m34 = l->m31 * r->m14 + l->m32 * r->m24 + l->m33 * r->m34 + l->m34 * r->m44;
    m->m44 = l->m41 * r->m14 + l->m42 * r->m24 + l->m43 * r->m34 + l->m44 * r->m44;
}

/*static void v4_mat3_mul(struct V4 *res, struct V4 *v, struct Mat4 *m)
{
    res->x = v->x*m->m11+v->y*m->m21+v->z*m->m31+v->w*m->m41;
    res->y = v->x*m->m12+v->y*m->m22+v->z*m->m32+v->w*m->m42;
    res->z = v->x*m->m13+v->y*m->m23+v->z*m->m33+v->w*m->m43;
    res->w = v->x*m->m14+v->y*m->m24+v->z*m->m34+v->w*m->m44;
}*/

// translate rotate scale
static inline void mat4_trs(struct Mat4 *res, struct V3 t, struct Quat r, struct V3 s)
{
    res->m11 = (1.0f-2.0f*(r.y*r.y+r.z*r.z))*s.x;
    res->m21 = (r.x*r.y+r.z*r.w)*s.x*2.0f;
    res->m31 = (r.x*r.z-r.y*r.w)*s.x*2.0f;
    res->m41 = 0.0f;
    res->m12 = (r.x*r.y-r.z*r.w)*s.y*2.0f;
    res->m22 = (1.0f-2.0f*(r.x*r.x+r.z*r.z))*s.y;
    res->m32 = (r.y*r.z+r.x*r.w)*s.y*2.0f;
    res->m42 = 0.0f;
    res->m13 = (r.x*r.z+r.y*r.w)*s.z*2.0f;
    res->m23 = (r.y*r.z-r.x*r.w)*s.z*2.0f;
    res->m33 = (1.0f-2.0f*(r.x*r.x+r.y*r.y))*s.z;
    res->m43 = 0.0f;
    res->m14 = t.x;
    res->m24 = t.y;
    res->m34 = t.z;
    res->m44 = 1.0f;
}

// translate rotate
static inline void mat4_tr(struct Mat4 *res, struct V3 t, struct Quat r)
{
    res->m11 = (1.0f-2.0f*(r.y*r.y+r.z*r.z));
    res->m21 = (r.x*r.y+r.z*r.w)*2.0f;
    res->m31 = (r.x*r.z-r.y*r.w)*2.0f;
    res->m41 = 0.0f;
    res->m12 = (r.x*r.y-r.z*r.w)*2.0f;
    res->m22 = (1.0f-2.0f*(r.x*r.x+r.z*r.z));
    res->m32 = (r.y*r.z+r.x*r.w)*2.0f;
    res->m42 = 0.0f;
    res->m13 = (r.x*r.z+r.y*r.w)*2.0f;
    res->m23 = (r.y*r.z-r.x*r.w)*2.0f;
    res->m33 = (1.0f-2.0f*(r.x*r.x+r.y*r.y));
    res->m43 = 0.0f;
    res->m14 = t.x;
    res->m24 = t.y;
    res->m34 = t.z;
    res->m44 = 1.0f;
}

// rotate translate
static inline void mat4_rt(struct Mat4 *res, struct Quat r, struct V3 t)
{
    Mat4 translate, rotate;
    mat4_identity(&translate);
    translate.m14 = t.x;
    translate.m24 = t.y;
    translate.m34 = t.z;
    mat4_rotation(&rotate, &r);
    mat4_mul(res, &rotate, &translate);
}

// translate scale
static inline void mat3_ts(Mat3 *restrict m, V2 translate, V2 scale)
{
   m->m11 = scale.x;
   m->m12 = 0.0f;
   m->m13 = scale.x*translate.x;
   m->m21 = 0.0f;
   m->m22 = scale.y;
   m->m23 = scale.y*translate.y;
   m->m31 = 0.0f;
   m->m32 = 0.0f;
   m->m33 = 1.0f;
}

static inline void mat3_v3_mul(V3 *restrict res, Mat3 *restrict m, V3 v)
{
    res->x = v.x * m->m11 + v.y * m->m12 + v.z * m->m13;
    res->y = v.x * m->m21 + v.y * m->m22 + v.z * m->m23;
    res->z = v.x * m->m31 + v.y * m->m32 + v.z * m->m33;
}

static inline void quat_v3_mul_pos(V3 *restrict res, Quat q, V3 v)
{
    // TODO: optimize
    Mat4 mat;
    mat4_rotation(&mat, &q);
    V4 vh = make_v4(v.x, v.y, v.z, 1.0f);
    V4 rh;
    mat4_v4_mul(&rh, &mat, vh);
    res->x = rh.x; res->y = rh.y; res->z = rh.z;
}

static inline void quat_v3_mul_dir(V3 *restrict res, Quat q, V3 v)
{
    // TODO: optimize
    Mat4 mat;
    mat4_rotation(&mat, &q);
    V4 vh = make_v4(v.x, v.y, v.z, 0.0f);
    V4 rh;
    mat4_v4_mul(&rh, &mat, vh);
    res->x = rh.x; res->y = rh.y; res->z = rh.z;
}
