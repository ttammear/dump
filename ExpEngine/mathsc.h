#pragma once

#include <math.h>
#include "libs/tt_types.h"

#define DEG2RAD_F 0.0174532925f

struct V2
{
    r32 x,y;
};

struct V3
{
    r32 x, y, z;
};

struct V4
{
    r32 x, y, z, w;
};

struct Quat
{
    r32 w, x, y, z;
};

struct Mat4
{
    union {
        struct
        {
            r32 m[16];
        };

        struct
        {
            r32 cr[4][4];
        };

        struct
        {
            r32 m11, m12, m13, m14;
            r32 m21, m22, m23, m24;
            r32 m31, m32, m33, m34;
            r32 m41, m42, m43, m44;
        };
    };
};

/*struct Mat4_simd
{
    TT_SIMD_T m11, m12, m13, m14;
    TT_SIMD_T m21, m22, m23, m24;
    TT_SIMD_T m31, m32, m33, m34;
    TT_SIMD_T m41, m42, m43, m44;
};

void mat4_simd_mul(struct Mat4_simd *result, struct Mat4_simd *l, struct Mat4 *r)
{

}*/

#ifdef AIKE_X86

struct Mat4_sse2
{
    union {
        struct {
            __m128 m11, m12, m13, m14;
            __m128 m21, m22, m23, m24;
            __m128 m31, m32, m33, m34;
            __m128 m41, m42, m43, m44;
        };
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
};

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

struct Mat4_sse2
{
    union {
        struct {
            struct Mat4 mat[4];
        };
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

static inline void quat_identity(struct Quat *q)
{
    q->w = 1.0f;
    q->x = 0.0f;
    q->y = 0.0f;
    q->z = 0.0f;
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

static inline void quat_euler_deg(struct Quat *q, struct V3 euler)
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
    m->m12 = 0.0f;
    m->m13 = 0.0f;
    m->m14 = 0.0f;

    m->m21 = 0.0f;
    m->m22 = 1.0f / tanHalfFOV;
    m->m23 = 0.0f;
    m->m24 = 0.0f;

    m->m31 = 0.0f;
    m->m32 = 0.0f;
    m->m33 = (-zNear - zFar) / depth;
    m->m34 = 1.0f;

    m->m41 = 0.0f;
    m->m42 = 0.0f;
    m->m43 = 2.0f * zFar * zNear / depth;
    m->m44 = 0.0f;
}

static inline void mat4_ortho(struct Mat4 *m, r32 l, r32 r, r32 b, r32 t, r32 zn, r32 zf)
{
    m->m11 = 2.0f / (r - l);
    m->m12 = 0.0f;
    m->m13 = 0.0f;
    m->m14 = 0.0f;

    m->m21 = 0.0f;
    m->m22 = 2.0f / (t - b);
    m->m23 = 0.0f;
    m->m24 = 0.0f;

    m->m31 = 0.0f;
    m->m32 = 0.0f;
    m->m33 = -2.0f/(zf-zn);
    m->m34 = 0.0f;

    m->m41 = -(r+l)/(r-l);
    m->m42 = -(t+b)/(t-b);
    m->m43 = -(zf+zn)/(zf-zn);
    m->m44 = 1.0f;
}

static inline void mat4_rotation(struct Mat4 *m, struct Quat *q)
{
    m->m11 = 1.0f - 2.0f*q->y*q->y - 2.0f*q->z*q->z;
	m->m12 = 2.0f*q->x*q->y + 2.0f*q->z*q->w;
	m->m13 = 2.0f*q->x*q->z - 2.0f*q->y*q->w;
	m->m14 = 0.0f;

	m->m21 = 2.0f*q->x*q->y - 2.0f*q->z*q->w;
	m->m22 = 1.0f - 2.0f*q->x*q->x - 2.0f*q->z*q->z;
	m->m23 = 2.0f*q->y*q->z + 2.0f*q->x*q->w;
	m->m24 = 0.0f;

	m->m31 = 2.0f*q->x*q->z + 2.0f*q->y*q->w;
	m->m32 = 2.0f*q->y*q->z - 2.0f*q->x*q->w;
	m->m33 = 1.0f - 2.0f*q->x*q->x - 2.0f*q->y*q->y;
	m->m34 = 0.0f;

	m->m41 = 0.0f;
	m->m42 = 0.0f;
	m->m43 = 0.0f;
	m->m44 = 1.0f;
}

static inline void mat4_identity(struct Mat4 *m)
{
    m->m11 = 1.0f; m->m12 = 0.0f; m->m13 = 0.0f; m->m14 = 0.0f;
    m->m21 = 0.0f; m->m22 = 1.0f; m->m23 = 0.0f; m->m24 = 0.0f;
    m->m31 = 0.0f; m->m32 = 0.0f; m->m33 = 1.0f; m->m34 = 0.0f;
    m->m41 = 0.0f; m->m42 = 0.0f; m->m43 = 0.0f; m->m44 = 1.0f;
}

static inline void mat4_mul(struct Mat4 *m, struct Mat4 *l, struct Mat4 *r)
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
    m->m11 = l->m11 * r->m11 + l->m21 * r->m12 + l->m31 * r->m13 + l->m41 * r->m14;
    m->m12 = l->m12 * r->m11 + l->m22 * r->m12 + l->m32 * r->m13 + l->m42 * r->m14;
    m->m13 = l->m13 * r->m11 + l->m23 * r->m12 + l->m33 * r->m13 + l->m43 * r->m14;
    m->m14 = l->m14 * r->m11 + l->m24 * r->m12 + l->m34 * r->m13 + l->m44 * r->m14;
    m->m21 = l->m11 * r->m21 + l->m21 * r->m22 + l->m31 * r->m23 + l->m41 * r->m24;
    m->m22 = l->m12 * r->m21 + l->m22 * r->m22 + l->m32 * r->m23 + l->m42 * r->m24;
    m->m23 = l->m13 * r->m21 + l->m23 * r->m22 + l->m33 * r->m23 + l->m43 * r->m24;
    m->m24 = l->m14 * r->m21 + l->m24 * r->m22 + l->m34 * r->m23 + l->m44 * r->m24;
    m->m31 = l->m11 * r->m31 + l->m21 * r->m32 + l->m31 * r->m33 + l->m41 * r->m34;
    m->m32 = l->m12 * r->m31 + l->m22 * r->m32 + l->m32 * r->m33 + l->m42 * r->m34;
    m->m33 = l->m13 * r->m31 + l->m23 * r->m32 + l->m33 * r->m33 + l->m43 * r->m34;
    m->m34 = l->m14 * r->m31 + l->m24 * r->m32 + l->m34 * r->m33 + l->m44 * r->m34;
    m->m41 = l->m11 * r->m41 + l->m21 * r->m42 + l->m31 * r->m43 + l->m41 * r->m44;
    m->m42 = l->m12 * r->m41 + l->m22 * r->m42 + l->m32 * r->m43 + l->m42 * r->m44;
    m->m43 = l->m13 * r->m41 + l->m23 * r->m42 + l->m33 * r->m43 + l->m43 * r->m44;
    m->m44 = l->m14 * r->m41 + l->m24 * r->m42 + l->m34 * r->m43 + l->m44 * r->m44;
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
    res->m12 = (r.x*r.y+r.z*r.w)*s.x*2.0f;
    res->m13 = (r.x*r.z-r.y*r.w)*s.x*2.0f;
    res->m14 = 0.0f;
    res->m21 = (r.x*r.y-r.z*r.w)*s.y*2.0f;
    res->m22 = (1.0f-2.0f*(r.x*r.x+r.z*r.z))*s.y;
    res->m23 = (r.y*r.z+r.x*r.w)*s.y*2.0f;
    res->m24 = 0.0f;
    res->m31 = (r.x*r.z+r.y*r.w)*s.z*2.0f;
    res->m32 = (r.y*r.z-r.x*r.w)*s.z*2.0f;
    res->m33 = (1.0f-2.0f*(r.x*r.x+r.y*r.y))*s.z;
    res->m34 = 0.0f;
    res->m41 = t.x;
    res->m42 = t.y;
    res->m43 = t.z;
    res->m44 = 1.0f;
}

// translate rotate
static inline void mat4_tr(struct Mat4 *res, struct V3 t, struct Quat r)
{
    res->m11 = (1.0f-2.0f*(r.y*r.y+r.z*r.z));
    res->m12 = (r.x*r.y+r.z*r.w)*2.0f;
    res->m13 = (r.x*r.z-r.y*r.w)*2.0f;
    res->m14 = 0.0f;
    res->m21 = (r.x*r.y-r.z*r.w)*2.0f;
    res->m22 = (1.0f-2.0f*(r.x*r.x+r.z*r.z));
    res->m23 = (r.y*r.z+r.x*r.w)*2.0f;
    res->m24 = 0.0f;
    res->m31 = (r.x*r.z+r.y*r.w)*2.0f;
    res->m32 = (r.y*r.z-r.x*r.w)*2.0f;
    res->m33 = (1.0f-2.0f*(r.x*r.x+r.y*r.y));
    res->m34 = 0.0f;
    res->m41 = t.x;
    res->m42 = t.y;
    res->m43 = t.z;
    res->m44 = 1.0f;
}
