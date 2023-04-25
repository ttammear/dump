
public struct IVec3
{
    public IVec3(int x, int y, int z)
    {
        this.x = x;
        this.y = y;
        this.z = z;
    }

    public static IVec3 operator +(IVec3 a, IVec3 b)
    {
        return new IVec3(a.x + b.x, a.y + b.y, a.z + b.z);
    }

    public static IVec3 operator -(IVec3 a, IVec3 b)
    {
        return new IVec3(a.x - b.x, a.y - b.y, a.z - b.z);
    }

    public static IVec3 operator *(int s, IVec3 b)
    {
        return new IVec3(s * b.x, s * b.y, s * b.z);
    }

    public static IVec3 operator *(IVec3 b, int s)
    {
        return new IVec3(s * b.x, s * b.y, s * b.z);
    }

    public int x,y,z;
}
