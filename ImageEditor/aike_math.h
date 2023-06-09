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

inline Vec2 operator / (Vec2 const &v, float m)
{
    return Vec2(v.x / m, v.y / m);
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

inline Vec3 operator + (Vec3 const &l, Vec3 const &r)
{
    return Vec3(l.x + r.x, l.y + r.y, l.z + r.z);
}

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

    inline static Mat4 proj2d(float w, float h)
    {
        Mat4 ret;
        ret.m00 = 2.0f / w; ret.m01 = 0.0f; ret.m02 = 0.0f; ret.m03 = -1.0f;
        ret.m10 = 0.0f; ret.m11 = -2.0f / h; ret.m12 = 0.0f; ret.m13 = 1.0f;
        ret.m20 = 0.0f; ret.m21 = 0.0f; ret.m22 = 1.0f; ret.m23 = 0.0f; // HMM this is not supposed to be 0??
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


struct Mat3
{
    Mat3() {}

    void transpose();
    Mat3 transposed();
    void sprint(char *buf, size_t bufSize);

    inline static Mat3 proj2d(float width, float height)
    {
        Mat3 ret;
        // first column
        ret.m00 = 2.0f / width; ret.m10 = 1.0f;             ret.m20 = 0.0f;
        // second column
        ret.m01 = 0.0f;         ret.m11 = -2.0f / height;   ret.m21 = 0.0f;
        // third column
        ret.m02 = -1.0f;        ret.m12 = 1.0f;             ret.m22 = 1.0f;
        return ret;
    }
    
    inline static Mat3 translateAndScale(Vec2 t, Vec2 s)
    {
        Mat3 ret;
        ret.m00 =     s.x; ret.m10 =    0.0f; ret.m20 =  0.0f;
        ret.m01 =    0.0f; ret.m11 =     s.y; ret.m21 =  0.0f;
        ret.m02 = s.x*t.x; ret.m12 = s.y*t.y; ret.m22 =  1.0f;
        return ret;
    }

    inline static Mat3 scaleAndTranslate(Vec2 s, Vec2 t)
    {
        Mat3 ret;
        ret.m00 =   s.x; ret.m10 =  0.0f; ret.m20 =  0.0f;
        ret.m01 =  0.0f; ret.m11 =   s.y; ret.m21 =  0.0f;
        ret.m02 =  t.x;  ret.m12 =   t.y; ret.m22 =  1.0f;
        return ret;
    }

    inline static Mat3 translate(Vec2 t)
    {
        Mat3 ret;
        ret.m00 =   1.0; ret.m10 =  0.0f; ret.m20 =  0.0f;
        ret.m01 =  0.0f; ret.m11 =  1.0f; ret.m21 =  0.0f;
        ret.m02 =   t.x; ret.m12 =   t.y; ret.m22 =  1.0f;
        return ret;
    }

    inline static Mat3 scale(Vec2 s)
    {
        Mat3 ret;
        ret.m00 =  s.x; ret.m10 =  0.0f; ret.m20 = 0.0f;
        ret.m01 = 0.0f; ret.m11 =   s.y; ret.m21 = 0.0f;
        ret.m02 = 0.0f; ret.m12 =  0.0f; ret.m22 = 1.0f;
        return ret;
    }

    inline static Mat3 identity()
    {
        Mat3 ret;
        ret.m00 = 1.0f; ret.m10 = 0.0f; ret.m20 = 0.0f;
        ret.m01 = 0.0f; ret.m11 = 1.0f; ret.m21 = 0.0f;
        ret.m02 = 0.0f; ret.m12 = 0.0f; ret.m22 = 1.0f;
        return ret;
    }

    union
    {
        float cr[3][3];
        float m[3][3];
        struct 
        {
            float m00, m10, m20,
                  m01, m11, m21,
                  m02, m12, m22;
        };
    };
};

inline void Mat3::transpose()
{
    for(int i = 0; i < 3; i++)
    for(int j = 0; j < 3; j++)
    {
        // TODO: huh? this assigns twice??
        float temp = this->m[i][j];
        this->m[i][j] = this->m[j][i];
        this->m[j][i] = temp;
    }
}

inline Mat3 Mat3::transposed()
{
    Mat3 ret;
    for(int i = 0; i < 3; i++)
    for(int j = 0; j < 3; j++)
    {
        ret.m[i][j] = this->m[j][i];
    }
    return ret;
}

inline void Mat3::sprint(char *buf, size_t bufSize)
{
    snprintf(buf, bufSize,  "%8.4f %8.4f %8.4f\r\n"
                            "%8.4f %8.4f %8.4f\r\n"
                            "%8.4f %8.4f %8.4f\r\n", 
                            this->m00, m01, m02,
                            this->m10, m11, m12,
                            this->m20, m21, m22);
}


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

Vec3 operator * (Mat3& mat, Vec3& vec)
{
    return Vec3(mat.m00*vec.x + mat.m01*vec.y + mat.m02*vec.z,
                mat.m10*vec.x + mat.m11*vec.y + mat.m12*vec.z,
                mat.m20*vec.x + mat.m21*vec.y + mat.m22*vec.z
            );
}

inline Mat3 operator * (Mat3 const &l, Mat3 const &r)
{
	Mat3 ret;
	for (int col = 0; col < 3; col++)
	{
		for (int row = 0; row < 3; row++)
		{
			ret.cr[col][row] = 0.0f;
			for (int n = 0; n < 3; n++)
			{
				ret.cr[col][row] += l.cr[n][row] * r.cr[col][n];
			}
		}
	}
	return ret;
}

#define IN_RECT(rectangle, point) (point.x > rectangle.x0 && point.x < rectangle.x0+rectangle.width && point.y > rectangle.y0 && point.y < rectangle.y0+rectangle.height)

#define CLAMP01(x) (x < 0.0f ? 0.0f : x > 1.0f ? 1.0f : x )

#define roundToInt(x) ((int)lround(x))
