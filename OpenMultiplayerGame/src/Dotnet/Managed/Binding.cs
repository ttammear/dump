using System;
using System.Runtime.InteropServices;

public enum NativeFunction : int
{    
    TransformSetPosition = 0,
    TransformGetPosition = 1,
    WorldCreateEntity = 2,
    WorldPtr = 3,
    TransformSetRotation = 4,
    TransformGetRotation = 5,
    EntityAddComponent = 6,
    BtRigidBodySetPosition = 7,
    BtRigidBodySetRotation = 8

}

public enum NativeComponent : int
{
    BtRigidBody
}

public struct Vec3
{
    public Vec3(float x, float y, float z)
    {
        this.x = x;
        this.y = y;
        this.z = z;
    }

    public float x,y,z;
}

public struct Quaternion
{
    public Quaternion(float w, float x, float y, float z)
    {
        this.w = w;
        this.x = x;
        this.y = y;
        this.z = z;
    }

    public static Quaternion AngleAxis(float angleDeg, Vec3 axis)
    {
		float halfAngleRad = (0.0174532925f*angleDeg)/2.0f;
		float sinHAR = (float)Math.Sin(halfAngleRad);
		return new Quaternion((float)Math.Cos(halfAngleRad), axis.x * sinHAR, axis.y * sinHAR, axis.z * sinHAR);
    }

    public float w,x,y,z;
}

public class Transform
{
    internal Transform(IntPtr nativePtr)
    {
        this.nativePtr = nativePtr;
    }

    public Vec3 position
    {
        get
        {
            return Binding.transformGetPosition(nativePtr);
        }

        set
        {
            Binding.transformSetPosition(nativePtr, ref value);
        }
    }

    public Quaternion rotation
    {
        get
        {
            return Binding.transformGetRotation(nativePtr);
        }
        set
        {
            Binding.transformSetRotation(nativePtr, ref value);
        }
    }

    protected IntPtr nativePtr;
}

public abstract class Component
{
    int whatever;
    internal abstract bool Initialize(Entity entity);
}

public class BtRigidBody : Component
{
    internal override bool Initialize(Entity entity)
    {
        IntPtr res = Binding.entityAddComponent(entity.nativePtr, (int)NativeComponent.BtRigidBody);
        nativePtr = res;
        return res != new IntPtr(0);
    }

    public Vec3 position
    {
        set
        {
            Binding.btRigidBodySetPosition(nativePtr, ref value);
        }
    }

    public Quaternion rotation
    {
        set
        {
            Binding.btRigidBodySetRotation(nativePtr, ref value);
        }
    }

    internal IntPtr nativePtr;
}

public class Entity
{
    internal Entity(IntPtr entityPtr, IntPtr transformPtr)
    {
        nativePtr = entityPtr;
        transform = new Transform(transformPtr);
    }

    public T AddComponent<T>() where T : Component, new()
    {
        T ret = new T();
        bool success = ret.Initialize(this);
        return success ? ret : default(T);
    }

    public Transform transform;
    internal IntPtr nativePtr;
}

public static class World
{
    internal static void SetWorld(IntPtr worldPtr)
    {
        nativePtr = worldPtr;
    }
    public static Entity CreateEntity()
    {
        // TODO: native call world->createEntity
        IntPtr entityPtr = new IntPtr(0);
        IntPtr transformPtr = new IntPtr(0);
        bool result = Binding.worldCreateEntity(nativePtr, ref entityPtr, ref transformPtr);
        if(result)
        {
            return new Entity(entityPtr, transformPtr);
        }
        return null;
    }
    static IntPtr nativePtr;
}

internal class Binding
{
    [UnmanagedFunctionPointer(CallingConvention.ThisCall)]
    public delegate void transformSetPosDelegate(IntPtr thisptr, ref Vec3 pos);
    [UnmanagedFunctionPointer(CallingConvention.ThisCall)]
    public delegate Vec3 transformGetPosDelegate(IntPtr thisptr);
    [UnmanagedFunctionPointer(CallingConvention.ThisCall)]
    public delegate bool worldCreateEntityDelegate(IntPtr thisptr, ref IntPtr entityPtr, ref IntPtr transformPtr);
    [UnmanagedFunctionPointer(CallingConvention.ThisCall)]
    public delegate void transformSetRotDelegate(IntPtr thisptr, ref Quaternion rot);
    [UnmanagedFunctionPointer(CallingConvention.ThisCall)]
    public delegate Quaternion transformGetRotDelegate(IntPtr thisptr);
    [UnmanagedFunctionPointer(CallingConvention.ThisCall)]
    public delegate IntPtr entityAddComponentDelegate(IntPtr thisptr, int component);
    [UnmanagedFunctionPointer(CallingConvention.ThisCall)]
    public delegate void btRigidBodySetPosDelegate(IntPtr thisptr, ref Vec3 pos);
    [UnmanagedFunctionPointer(CallingConvention.ThisCall)]
    public delegate void btRigidBodySetRotDelegate(IntPtr thisptr, ref Quaternion rot);

    public static bool Initialized = false;
    public static transformSetPosDelegate transformSetPosition;
    public static transformGetPosDelegate transformGetPosition;
    public static transformSetRotDelegate transformSetRotation;
    public static transformGetRotDelegate transformGetRotation;
    public static worldCreateEntityDelegate worldCreateEntity;
    public static entityAddComponentDelegate entityAddComponent;
    public static btRigidBodySetPosDelegate btRigidBodySetPosition;
    public static btRigidBodySetRotDelegate btRigidBodySetRotation;
    static IntPtr trans;

    public static void setPtr(NativeFunction funType, IntPtr func, IntPtr transform)
    {
        switch(funType)
        {
            case NativeFunction.TransformSetPosition:
                transformSetPosition = (transformSetPosDelegate)Marshal.GetDelegateForFunctionPointer(func, typeof(transformSetPosDelegate));
                break;
            case NativeFunction.TransformGetPosition:
                transformGetPosition = (transformGetPosDelegate)Marshal.GetDelegateForFunctionPointer(func, typeof(transformGetPosDelegate));
                break;
            case NativeFunction.WorldCreateEntity:
                worldCreateEntity = (worldCreateEntityDelegate)Marshal.GetDelegateForFunctionPointer(func, typeof(worldCreateEntityDelegate));
                break;
            case NativeFunction.TransformSetRotation:
                transformSetRotation = (transformSetRotDelegate)Marshal.GetDelegateForFunctionPointer(func, typeof(transformSetRotDelegate));
                break;
            case NativeFunction.TransformGetRotation:
                transformGetRotation = (transformGetRotDelegate)Marshal.GetDelegateForFunctionPointer(func, typeof(transformGetRotDelegate));
                break;
            case NativeFunction.WorldPtr:
                World.SetWorld(func);
                break;
            case NativeFunction.EntityAddComponent:
                entityAddComponent = (entityAddComponentDelegate)Marshal.GetDelegateForFunctionPointer(func, typeof(entityAddComponentDelegate));
                break;
            case NativeFunction.BtRigidBodySetPosition:
                btRigidBodySetPosition = (btRigidBodySetPosDelegate)Marshal.GetDelegateForFunctionPointer(func, typeof(btRigidBodySetPosDelegate));
                break;
            case NativeFunction.BtRigidBodySetRotation:
                btRigidBodySetRotation = (btRigidBodySetRotDelegate)Marshal.GetDelegateForFunctionPointer(func, typeof(btRigidBodySetRotDelegate));
                break;
            default:
                Console.Error.WriteLine("Unknown function pointer passed to managed code: {0}", (int)funType);
                break;
        }
        trans = transform;
    }

    static Entity ent;
    static BtRigidBody rbComp;

    public static void testing()
    {
        System.Console.WriteLine("Hello 222 from C# !!!");
        Vec3 got = transformGetPosition(trans);
        System.Console.WriteLine("original {0} {1} {2}", got.x, got.y, got.z);
        Vec3 pos = new Vec3(0.0f, 5.0f, 0.0f);
        transformSetPosition(trans, ref pos);

        ent = World.CreateEntity();
        ent.transform.position = new Vec3(0.0f, 10.0f, 0.0f);
        ent.transform.rotation = Quaternion.AngleAxis(45.0f, new Vec3(0.0f, 1.0f, 0.0f));
        rbComp = ent.AddComponent<BtRigidBody>();

        ent = World.CreateEntity();
        ent.transform.position = new Vec3(0.0f, 5.0f, 0.0f);

        rbComp.position = new Vec3(0.0f, 100.0f, 0.0f);
    }

    static float angle = 0.0f;
    public static void update()
    {
        if(ent == null)
            return;
        ent.transform.rotation = Quaternion.AngleAxis(angle, new Vec3(0.0f, 1.0f, 0.0f));
        angle += 1.0f / 60.0f * 180.0f;
    }
}