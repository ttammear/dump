#pragma once 

struct Vec2
{
    Vec2() {}

    Vec2(float x, float y)
    {
        this->x = x;
        this->y = y;
    }

    union {
        struct
        {
            float x,y;
        };
        struct
        {
            float v[2];
        };
    };
};

inline Vec2 operator + (Vec2 const &l, Vec2 const &r)
{
	return Vec2(l.x + r.x, l.y + r.y);
}

inline Vec2 operator - (Vec2 const &l, Vec2 const &r)
{
	return Vec2(l.x - r.x, l.y - r.y);
}

inline Vec2 operator * (float m, Vec2 const &v)
{
    return Vec2(m*v.x, m*v.y);
}

inline Vec2 operator * (Vec2 const &v, float m)
{
    return Vec2(m*v.x, m*v.y);
}


struct Vec3
{
    Vec3() {}

    Vec3(float x, float y, float z)
    {
        this->x = x;
        this->y = y;
        this->z = z;
    }

    union {
        struct
        {
            float x,y,z;
        };
        struct
        {
            float r,g,b;
        };
        struct
        {
            float v[3];
        };
    };
};

struct Vec4
{
    Vec4() {}

    Vec4(float x, float y, float z, float w)
    {
        this->x = x;
        this->y = y;
        this->z = z;
        this->w = w;
    }

    union {
        struct
        {
            float x,y,z,w;
        };
        struct
        {
            float r,g,b,a;
        };
        struct
        {
            float v[4];
        };
    };
};

struct Mat4 // column major
{
    Mat4() {}

    inline static Mat4 proj2d(float width, float height)
    {
        Mat4 ret;
        ret.m00 = 2.0f / width; ret.m01 = 0.0f; ret.m02 = 0.0f; ret.m03 = -1.0f;
        ret.m10 = 0.0f; ret.m11 = -2.0f / height; ret.m12 = 0.0f; ret.m13 = 1.0f;
        ret.m20 = 0.0f; ret.m21 = 0.0f; ret.m22 = 1.0f; ret.m23 = 0.0f;
        ret.m30 = 0.0f; ret.m31 = 0.0f; ret.m32 = 0.0f; ret.m33 = 1.0f;
        return ret;
    }

    union
    {
        float m[4][4];
        struct
        {
            float m00, m10, m20, m30,
                  m01, m11, m21, m31,
                  m02, m12, m22, m32,
                  m03, m13, m23, m33;
        };

    };
};

struct Rect
{
    Rect() {}

    Rect(Vec2 origin, Vec2 size)
    {
        this->x0 = origin.x;
        this->y0 = origin.y;
        this->width = size.x;
        this->height = size.y;
    }

    Rect(Vec2 origin, float width, float height)
    {
        this->x0 = origin.x;
        this->y0 = origin.y;
        this->width = width;
        this->height = height;
    }

    Rect(float x0, float y0, float width, float height)
    {
        this->x0 = x0;
        this->y0 = y0;
        this->width = width;
        this->height = height;
    }

    float x0, y0, width, height;
};

#define IN_RECT(rectangle, point) (point.x > rectangle.x0 && point.x < rectangle.x0+rectangle.width && point.y > rectangle.y0 && point.y < rectangle.y0+rectangle.height)

#define CLAMP01(x) (x < 0.0f ? 0.0f : x > 1.0f ? 1.0f : x )

#define roundToInt(x) ((int)lround(x))
