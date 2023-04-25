/*
 * This is free and unencumbered software released into the public domain.
 *
 * Anyone is free to copy, modify, publish, use, compile, sell, or
 * distribute this software, either in source code form or as a compiled
 * binary, for any purpose, commercial or non-commercial, and by any
 * means.
 *
 * In jurisdictions that recognize copyright laws, the author or authors
 * of this software dedicate any and all copyright interest in the
 * software to the public domain. We make this dedication for the benefit
 * of the public at large and to the detriment of our heirs and
 * successors. We intend this dedication to be an overt act of
 * relinquishment in perpetuity of all present and future rights to this
 * software under copyright law.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * For more information, please refer to <http://unlicense.org/>
 */

using System;
using UnityEngine;

struct SMat3
{
    public float m00, m01, m02, m11, m12, m22;

    public SMat3(float m00, float m01, float m02,
          float m11, float m12, float m22)
    {
        this.m00 = m00;
        this.m01 = m01;
        this.m02 = m02;
        this.m11 = m11;
        this.m12 = m12;
        this.m22 = m22;
    }

    public void Clear()
    {
        SetSymmetric(0, 0, 0, 0, 0, 0);
    }

    public void SetSymmetric(ref SMat3 rhs)
    {
        SetSymmetric(rhs.m00, rhs.m01, rhs.m02, rhs.m11, rhs.m12, rhs.m22);
    }

    public void SetSymmetric(float m00, float m01, float m02, float m11, float m12, float m22)
    {
        this.m00 = m00;
        this.m01 = m01;
        this.m02 = m02;
        this.m11 = m11;
        this.m12 = m12;
        this.m22 = m22;
    }

    public static Vector3 vmul_symmetric(ref SMat3 a, Vector3 v)
    {
        Vector3 ret;
        ret.x = (a.m00 * v.x) + (a.m01 * v.y) + (a.m02 * v.z);
        ret.y = (a.m01 * v.x) + (a.m11 * v.y) + (a.m12 * v.z);
        ret.z = (a.m02 * v.x) + (a.m12 * v.y) + (a.m22 * v.z);
        return ret;
    }
};

struct Mat3
{
    public float m00, m01, m02,
        m10, m11, m12,
        m20, m21, m22;

    public Mat3(float m00, float m01, float m02, float m10, float m11, float m12, float m20, float m21, float m22)
    {
        this.m00 = m00;
        this.m01 = m01;
        this.m02 = m02;
        this.m10 = m10;
        this.m11 = m11;
        this.m12 = m12;
        this.m20 = m20;
        this.m21 = m21;
        this.m22 = m22;
    }

    static public Vector3 vmul(ref Mat3 a, Vector3 v)
    {
        Vector3 ret;
        ret.x = (a.m00 * v.x) + (a.m01 * v.y) + (a.m02 * v.z);
        ret.y = (a.m10 * v.x) + (a.m11 * v.y) + (a.m12 * v.z);
        ret.z = (a.m20 * v.x) + (a.m21 * v.y) + (a.m22 * v.z);
        return ret;
    }



    public void SetSymmetric(float a00, float a01, float a02,
                            float a11, float a12, float a22)
    {
        this.Set(a00, a01, a02, a01, a11, a12, a02, a12, a22);
    }

    public void Set(float m00, float m01, float m02,
                   float m10, float m11, float m12,
                   float m20, float m21, float m22)
    {
        this.m00 = m00;
        this.m01 = m01;
        this.m02 = m02;
        this.m10 = m10;
        this.m11 = m11;
        this.m12 = m12;
        this.m20 = m20;
        this.m21 = m21;
        this.m22 = m22;
    }

    public void SetSymmetric(ref SMat3 rhs)
    {
        SetSymmetric(rhs.m00, rhs.m01, rhs.m02, rhs.m11, rhs.m12, rhs.m22);
    }
}

public class QefData
{
    public float ata_00, ata_01, ata_02, ata_11, ata_12, ata_22;
    public float atb_x, atb_y, atb_z;
    public float btb;
    public float massPoint_x, massPoint_y, massPoint_z;
    public int numPoints;

    public QefData()
    {
        Clear();
    }

    public QefData(float ata_00, float ata_01,
                     float ata_02, float ata_11, float ata_12,
                     float ata_22, float atb_x, float atb_y,
                     float atb_z, float btb, float massPoint_x,
                     float massPoint_y, float massPoint_z,
                     int numPoints)
    {
        Set(ata_00, ata_01, ata_02, ata_11, ata_12, ata_22, atb_x, atb_y,
            atb_z, btb, massPoint_x, massPoint_y, massPoint_z, numPoints);
    }

    public QefData(QefData rhs)
    {
        Set(rhs);
    }

    public void Add(QefData rhs)
    {
        this.ata_00 += rhs.ata_00;
        this.ata_01 += rhs.ata_01;
        this.ata_02 += rhs.ata_02;
        this.ata_11 += rhs.ata_11;
        this.ata_12 += rhs.ata_12;
        this.ata_22 += rhs.ata_22;
        this.atb_x += rhs.atb_x;
        this.atb_y += rhs.atb_y;
        this.atb_z += rhs.atb_z;
        this.btb += rhs.btb;
        this.massPoint_x += rhs.massPoint_x;
        this.massPoint_y += rhs.massPoint_y;
        this.massPoint_z += rhs.massPoint_z;
        this.numPoints += rhs.numPoints;
    }

    public void Clear()
    {
        Set(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
    }

    public void Set(float ata_00, float ata_01,
                  float ata_02, float ata_11, float ata_12,
                  float ata_22, float atb_x, float atb_y,
                  float atb_z, float btb, float massPoint_x,
                  float massPoint_y, float massPoint_z,
                  int numPoints)
    {
        this.ata_00 = ata_00;
        this.ata_01 = ata_01;
        this.ata_02 = ata_02;
        this.ata_11 = ata_11;
        this.ata_12 = ata_12;
        this.ata_22 = ata_22;
        this.atb_x = atb_x;
        this.atb_y = atb_y;
        this.atb_z = atb_z;
        this.btb = btb;
        this.massPoint_x = massPoint_x;
        this.massPoint_y = massPoint_y;
        this.massPoint_z = massPoint_z;
        this.numPoints = numPoints;
    }

    public void Set(QefData rhs)
    {
        Set(rhs.ata_00, rhs.ata_01, rhs.ata_02, rhs.ata_11, rhs.ata_12,
                  rhs.ata_22, rhs.atb_x, rhs.atb_y, rhs.atb_z, rhs.btb,
                  rhs.massPoint_x, rhs.massPoint_y, rhs.massPoint_z,
                  rhs.numPoints);
    }
}

public class QefSolver
{
    QefData data;
    SMat3 ata;
    Vector3 atb, x;
    bool hasSolution;
    public Vector3 massPoint;

    public QefSolver()
    {
        data = new QefData();
        ata = new SMat3(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
        atb = Vector3.zero;
        massPoint = Vector3.zero;
        x = Vector3.zero;
        hasSolution = false;
    }

    public static void normalize(ref float x, ref float y, ref float z)
    {
        Vector3 tmp = new Vector3(x, y, z);
        float d2 = Vector3.Dot(tmp, tmp);
        if(d2 < 1e-12)
        {
            x = 0.0f;
            y = 0.0f;
            z = 0.0f;
        }
        else
        {
            float d = Mathf.Sqrt(d2);
            x /= d;
            y /= d;
            z /= d;
        }
    }

    public void Add(float px, float py, float pz,
                        float nx, float ny, float nz)
    {
        this.hasSolution = false;
        normalize(ref nx, ref ny, ref nz);
        this.data.ata_00 += nx * nx;
        this.data.ata_01 += nx * ny;
        this.data.ata_02 += nx * nz;
        this.data.ata_11 += ny * ny;
        this.data.ata_12 += ny * nz;
        this.data.ata_22 += nz * nz;
        float dot = nx * px + ny * py + nz * pz;
        this.data.atb_x += dot * nx;
        this.data.atb_y += dot * ny;
        this.data.atb_z += dot * nz;
        this.data.btb += dot * dot;
        this.data.massPoint_x += px;
        this.data.massPoint_y += py;
        this.data.massPoint_z += pz;
        ++this.data.numPoints;
    }

    public void Add(Vector3 p, Vector3 n)
    {
        Add(p.x, p.y, p.z, n.x, n.y, n.z);
    }

    public void Add(QefData rhs)
    {
        hasSolution = false;
        data.Add(rhs);
    }

	public QefData GetData()
	{
		return data;
	}

    public float GetError()
    {
        if (!this.hasSolution) {
            throw new InvalidProgramException();
        }
        return this.GetError(this.x);
    }

    public float GetError(Vector3 pos)
    {
        if (!this.hasSolution) {
            this.SetAta();
            this.SetAtb();
        }

        Vector3 atax = SMat3.vmul_symmetric(ref this.ata, pos);
        return Vector3.Dot(pos, atax) - 2 * Vector3.Dot(pos, atb)
               + data.btb;
    }

    public void Reset()
    {
        hasSolution = false;
        data.Clear();
    }

    public void SetAta()
    {
        ata.SetSymmetric(data.ata_00, data.ata_01,
                               data.ata_02, data.ata_11, data.ata_12,
                               data.ata_22);
    }

    public void SetAtb()
    {
        atb.Set(data.atb_x, data.atb_y, data.atb_z);
    }

    static void calcSymmetricGivensCoefficients(float a_pp,
            float a_pq, float a_qq, out float c, out float s)
    {
        if (a_pq == 0) {
            c = 1;
            s = 0;
            return;
        }

        float tau = (a_qq - a_pp) / (2 * a_pq);
        float stt = Mathf.Sqrt(1.0f + tau * tau);
        float tan = 1.0f / ((tau >= 0) ? (tau + stt) : (tau - stt));
        c = 1.0f / Mathf.Sqrt(1.0f + tan * tan);
        s = tan * c;
    }

    static void rot01(ref SMat3 m, out float c, out float s)
    {
        calcSymmetricGivensCoefficients(m.m00, m.m01, m.m11, out c, out s);
        float cc = c * c;
        float ss = s * s;
        float mix = 2 * c * s * m.m01;
        m.SetSymmetric(cc * m.m00 - mix + ss * m.m11, 0, c * m.m02 - s * m.m12,
                       ss * m.m00 + mix + cc * m.m11, s * m.m02 + c * m.m12, m.m22);
    }

    static void rot02(ref SMat3 m, out float c, out float s)
    {
        calcSymmetricGivensCoefficients(m.m00, m.m02, m.m22, out c, out s);
        float cc = c * c;
        float ss = s * s;
        float mix = 2 * c * s * m.m02;
        m.SetSymmetric(cc * m.m00 - mix + ss * m.m22, c * m.m01 - s * m.m12, 0,
                       m.m11, s * m.m01 + c * m.m12, ss * m.m00 + mix + cc * m.m22);
    }

    static void rot12(ref SMat3 m, out float c, out float s)
    {
        calcSymmetricGivensCoefficients(m.m11, m.m12, m.m22, out c, out s);
        float cc = c * c;
        float ss = s * s;
        float mix = 2 * c * s * m.m12;
        m.SetSymmetric(m.m00, c * m.m01 - s * m.m02, s * m.m01 + c * m.m02,
                       cc * m.m11 - mix + ss * m.m22, 0, ss * m.m11 + mix + cc * m.m22);
    }

    static void rot01_post(ref Mat3 m, float c, float s)
    {
        float m00 = m.m00, m01 = m.m01, m10 = m.m10, m11 = m.m11, m20 = m.m20,
                     m21 = m.m21;
        m.Set(c * m00 - s * m01, s * m00 + c * m01, m.m02, c * m10 - s * m11,
              s * m10 + c * m11, m.m12, c * m20 - s * m21, s * m20 + c * m21, m.m22);
    }

    static void rot02_post(ref Mat3 m, float c, float s)
    {
        float m00 = m.m00, m02  = m.m02, m10  = m.m10, m12  = m.m12,
                     m20 = m.m20, m22  = m.m22 ;
        m.Set(c * m00 - s * m02, m.m01, s * m00 + c * m02, c * m10 - s * m12, m.m11,
              s * m10 + c * m12, c * m20 - s * m22, m.m21, s * m20 + c * m22);
    }

    static void rot12_post(ref Mat3 m, float c, float s)
    {
        float m01  = m.m01, m02  = m.m02, m11  = m.m11, m12  = m.m12,
                     m21  = m.m21, m22  = m.m22;
        m.Set(m.m00, c * m01 - s * m02, s * m01 + c * m02, m.m10, c * m11 - s * m12,
              s * m11 + c * m12, m.m20, c * m21 - s * m22, s * m21 + c * m22);
    }

    static void rotate01(ref SMat3 vtav, ref Mat3 v)
    {
        if (vtav.m01 == 0) {
            return;
        }

        float c, s;
        rot01(ref vtav, out c, out s);
        rot01_post(ref v, c, s);
    }

    static void rotate02(ref SMat3 vtav, ref Mat3 v)
    {
        if (vtav.m02 == 0) {
            return;
        }

        float c, s;
        rot02(ref vtav, out c, out s);
        rot02_post(ref v, c, s);
    }

    static void rotate12(ref SMat3 vtav, ref Mat3 v)
    {
        if (vtav.m12 == 0) {
            return;
        }

        float c, s;
        rot12(ref vtav, out c, out s);
        rot12_post(ref v, c, s);
    }

    float off(ref Mat3 a)
    {
        return Mathf.Sqrt((a.m01 * a.m01) + (a.m02 * a.m02) + (a.m10 * a.m10)
                    + (a.m12 * a.m12) + (a.m20 * a.m20) + (a.m21 * a.m21));
    }

    float fnorm(ref Mat3 a)
    {
        return Mathf.Sqrt((a.m00 * a.m00) + (a.m01 * a.m01) + (a.m02 * a.m02)
                    + (a.m10 * a.m10) + (a.m11 * a.m11) + (a.m12 * a.m12)
                    + (a.m20 * a.m20) + (a.m21 * a.m21) + (a.m22 * a.m22));
    }

    float fnorm(ref SMat3 a)
    {
        return Mathf.Sqrt((a.m00 * a.m00) + (a.m01 * a.m01) + (a.m02 * a.m02)
                    + (a.m01 * a.m01) + (a.m11 * a.m11) + (a.m12 * a.m12)
                    + (a.m02 * a.m02) + (a.m12 * a.m12) + (a.m22 * a.m22));
    }

    float off(ref SMat3 a)
    {
        return Mathf.Sqrt(2 * ((a.m01 * a.m01) + (a.m02 * a.m02) + (a.m12 * a.m12)));
    }

    void getSymmetricSvd(ref SMat3 a, ref SMat3 vtav, ref Mat3 v,
                              float tol,
                              int max_sweeps)
    {
        vtav.SetSymmetric(ref a);
        v.Set(1, 0, 0, 0, 1, 0, 0, 0, 1);
        float delta = tol * fnorm(ref vtav);

        for (int i = 0; i < max_sweeps
                && off(ref vtav) > delta; ++i) {
            rotate01(ref vtav, ref v);
            rotate02(ref vtav, ref v);
            rotate12(ref vtav, ref v);
        }
    }

    static float pinv(float x, float tol)
    {
        return (Mathf.Abs(x) < tol || Mathf.Abs(1.0f / x) < tol) ? 0.0f : (1.0f / x);
    }

    void pseudoinverse(ref Mat3 outM, ref SMat3 d, ref Mat3 v,
                            float tol)
    {
        float d0 = pinv(d.m00, tol), d1 = pinv(d.m11, tol), d2 = pinv(d.m22,
                          tol);
        outM.Set(v.m00 * d0 * v.m00 + v.m01 * d1 * v.m01 + v.m02 * d2 * v.m02,
                v.m00 * d0 * v.m10 + v.m01 * d1 * v.m11 + v.m02 * d2 * v.m12,
                v.m00 * d0 * v.m20 + v.m01 * d1 * v.m21 + v.m02 * d2 * v.m22,
                v.m10 * d0 * v.m00 + v.m11 * d1 * v.m01 + v.m12 * d2 * v.m02,
                v.m10 * d0 * v.m10 + v.m11 * d1 * v.m11 + v.m12 * d2 * v.m12,
                v.m10 * d0 * v.m20 + v.m11 * d1 * v.m21 + v.m12 * d2 * v.m22,
                v.m20 * d0 * v.m00 + v.m21 * d1 * v.m01 + v.m22 * d2 * v.m02,
                v.m20 * d0 * v.m10 + v.m21 * d1 * v.m11 + v.m22 * d2 * v.m12,
                v.m20 * d0 * v.m20 + v.m21 * d1 * v.m21 + v.m22 * d2 * v.m22);
    }

    static float calcError(ref Mat3 A, Vector3 x,
                            Vector3 b)
    {
        Vector3 vtmp = Mat3.vmul(ref A, x);
        vtmp = b - vtmp;
        return Vector3.Dot(vtmp, vtmp);
    }

    static float calcError(ref SMat3 origA, Vector3 x,
                            Vector3 b)
    {
        Mat3 A = new Mat3(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
        Vector3 vtmp;
        A.SetSymmetric(ref origA);
        vtmp = Mat3.vmul(ref A, x);
        vtmp = b - vtmp;
        return Vector3.Dot(vtmp, vtmp);
    }

    float solveSymmetric(ref SMat3 A, ref Vector3 b, ref Vector3 x,
                               float svd_tol, int svd_sweeps, float pinv_tol)
    {
        Mat3 V;
        SMat3 VTAV = new SMat3(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
        V = new Mat3(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
        var pinv = new Mat3(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
        getSymmetricSvd(ref A, ref VTAV, ref V, svd_tol, svd_sweeps);
        pseudoinverse(ref pinv, ref VTAV, ref V, pinv_tol);
        x = Mat3.vmul(ref pinv, b);
        return calcError(ref A, x, b);
    }

    public float Solve(out Vector3 outx, float svd_tol,
                            int svd_sweeps, float pinv_tol)
    {
        if (data.numPoints == 0) {
            throw new InvalidOperationException();
        }

        this.massPoint.Set(data.massPoint_x, data.massPoint_y,
                            data.massPoint_z);
        massPoint *= 1.0f / data.numPoints;
        SetAta();
        SetAtb();
        Vector3 tmpv = SMat3.vmul_symmetric(ref ata, massPoint);
        this.atb = this.atb - tmpv;
        x = Vector3.zero;
        float result = solveSymmetric(ref ata, ref atb,
                              ref x, svd_tol, svd_sweeps, pinv_tol);
        x = x + this.massPoint;
        SetAtb();
        outx = x;
        hasSolution = true;
        return result;
    }
}
