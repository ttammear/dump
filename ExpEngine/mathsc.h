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

static inline void quat_angle_axis(struct Quat *q, r32 angleDeg, struct V3 axis)
{
    r32 halfAngleRad = (DEG2RAD_F*angleDeg) / 2.0f;
    r32 sinHAR = sinf(halfAngleRad);
    q->w = cosf(halfAngleRad);
    q->x = axis.x * sinHAR;
    q->y = axis.y * sinHAR;
    q->z = axis.z * sinHAR;
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
	for (int col = 0; col < 4; col++)
	{
		for (int row = 0; row < 4; row++)
		{
			m->cr[col][row] = 0.0f;
			for (int n = 0; n < 4; n++)
			{
				m->cr[col][row] += l->cr[n][row] * r->cr[col][n];
			}
		}
	}
}

static void v4_mat3_mul(struct V4 *res, struct V4 *v, struct Mat4 *m)
{
    res->x = v->x*m->m11+v->y*m->m21+v->z*m->m31+v->w*m->m41;
    res->y = v->x*m->m12+v->y*m->m22+v->z*m->m32+v->w*m->m42;
    res->z = v->x*m->m13+v->y*m->m23+v->z*m->m33+v->w*m->m43;
    res->w = v->x*m->m14+v->y*m->m24+v->z*m->m34+v->w*m->m44;
}

static inline void mat4_trs(struct Mat4 *m, struct V3 t, struct Quat r, struct V3 s)
{
    struct Mat4 translate;
    mat4_identity(&translate);
    translate.m41 = t.x;
    translate.m42 = t.y;
    translate.m43 = t.z;

    struct Mat4 rotate;
    mat4_rotation(&rotate, &r);

    struct Mat4 scale;
    mat4_identity(&scale);
    scale.m11 = s.x;
    scale.m22 = s.y;
    scale.m33 = s.z;

    struct Mat4 transRot;
    mat4_mul(&transRot, &translate, &rotate);
    mat4_mul(m, &transRot, &scale);
}
