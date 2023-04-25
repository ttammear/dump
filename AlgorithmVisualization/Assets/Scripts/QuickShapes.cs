using System.Collections;
using System.Collections.Generic;
using System.Linq;
using UnityEngine;
using UnityEngine.Profiling;
using System.Text;

[DefaultExecutionOrder(-1000)]
public class QuickShapes : MonoBehaviour
{
    static Mesh cubeMesh;
    static Mesh cylinderMesh;
    static Mesh sphereMesh;
    static Mesh coneMesh;

    static Material cubeMaterial;
    static Material gridMaterial;
    static Material vertColorNotInstanced;

    public Material CubeMaterial;
    public Material GridMaterial;
    public Material VertColorNotInstanced;

    static Mesh tempMesh;
    static List<Vector3> tempVertices;
    static List<Color> tempVertexColors;

    static Matrix4x4[] _lineMatrices;
    static Vector4[] _lineColors;
    static int _lineCount;

    static Matrix4x4[] _sphereMatrices;
    static Vector4[] _sphereColors;
    static int _sphereCount;

    static Matrix4x4[] _cubeMatrices;
    static Vector4[] _cubeColors;
    static int _cubeCount;

    static Matrix4x4[] _coneMatrices;
    static Vector4[] _coneColors;
    static int _coneCount;

    static List<Mesh> _deleteMeshes;

    private void Awake()
    {
        cubeMesh = new Mesh();
        cubeMesh.vertices = cubeVertices;
        cubeMesh.triangles = cubeIndices;
        cubeMesh.RecalculateNormals();
        cubeMesh.RecalculateBounds();

        coneMesh = new Mesh();
        coneMesh.vertices = coneVertices;
        coneMesh.triangles = coneIndices;
        coneMesh.RecalculateNormals();
        coneMesh.RecalculateBounds();

        cubeMaterial = CubeMaterial;
        gridMaterial = GridMaterial;
        vertColorNotInstanced = VertColorNotInstanced;

        GameObject go = GameObject.CreatePrimitive(PrimitiveType.Cylinder);
        cylinderMesh = go.GetComponent<MeshFilter>().sharedMesh;
        Destroy(go);
        go = GameObject.CreatePrimitive(PrimitiveType.Sphere);
        sphereMesh = go.GetComponent<MeshFilter>().sharedMesh;
        Destroy(go);

        tempMesh = new Mesh();
        tempMesh.MarkDynamic();

        _lineMatrices = new Matrix4x4[1023];
        _lineColors = new Vector4[1023];
        _lineCount = 0;

        _sphereMatrices = new Matrix4x4[1023];
        _sphereColors = new Vector4[1023];
        _sphereCount = 0;

        _cubeMatrices = new Matrix4x4[1023];
        _cubeColors = new Vector4[1023];
        _cubeCount = 0;

        _coneMatrices = new Matrix4x4[1023];
        _coneColors = new Vector4[1023];
        _coneCount = 0;

        tempVertices = new List<Vector3>();
        tempVertexColors = new List<Color>();

        _deleteMeshes = new List<Mesh>();
    }

    void OnDestroy()
    {
        if(cubeMesh != null)
            Destroy(cubeMesh);
        if(coneMesh != null)
            Destroy(coneMesh);

        // Set references to null so GC collects them
        cubeMesh = null;
        coneMesh = null;
        cylinderMesh = null;
        sphereMesh = null;
        tempMesh = null;

        cubeMaterial = null;
        gridMaterial = null;
        vertColorNotInstanced = null;

        _lineMatrices = null;
        _lineColors = null;
        _sphereMatrices = null;
        _sphereColors = null;
        _cubeMatrices = null;
        _cubeColors = null;
        _coneMatrices = null;
        _coneColors = null;
        
        tempVertices = null;
        tempVertexColors = null;
    }

    private void Update()
    {
        _lineCount = 0;
        _sphereCount = 0;
        _cubeCount = 0;
        _coneCount = 0;
        foreach (var mesh in _deleteMeshes)
            MonoBehaviour.Destroy(mesh);
    }

    private void LateUpdate()
    {
        RenderLines();
        RenderSpheres();
        RenderCubes();
        RenderCones();
        RenderVertices();
    }

    static void RenderVertices()
    {
        if (tempVertices.Count < 3)
            return;
        Profiler.BeginSample("Render Vertices");
        tempMesh.SetVertices(tempVertices);
        int[] indices = new int[tempVertices.Count];
        for (int i = 0; i < indices.Length; i++)
            indices[i] = i;
        tempMesh.triangles = indices;
        tempMesh.SetColors(tempVertexColors);
        tempMesh.RecalculateBounds();
        tempMesh.RecalculateNormals();
        tempVertices.Clear();
        tempVertexColors.Clear();
        vertColorNotInstanced.color = Color.white;
        Graphics.DrawMesh(tempMesh, Matrix4x4.identity, vertColorNotInstanced, 0);
        Profiler.EndSample();
    }

    static void RenderLines()
    {
        Profiler.BeginSample("Render lines");
        if (_lineCount > 0)
        {
            var props = new MaterialPropertyBlock();
            props.SetVectorArray("_Color", _lineColors);
            Graphics.DrawMeshInstanced(cylinderMesh, 0, gridMaterial, _lineMatrices, _lineCount, props);
            _lineCount = 0;
        }
        Profiler.EndSample();
    }

    public static void DrawLine(Vector3 start, Vector3 end, float radius, Color color)
    {
        float length = Vector3.Distance(start, end);
        _lineMatrices[_lineCount] = Matrix4x4.TRS(start + (end - start) * 0.5f, Quaternion.LookRotation((start - end).normalized) * Quaternion.AngleAxis(90.0f, Vector3.right), new Vector3(radius, length / 2.0f, radius));
        _lineColors[_lineCount] = new Vector4(color.r, color.g, color.b, color.a);
        _lineCount++;
        if (_lineCount >= 1022)
        {
            RenderLines();
        }
    }

    static void RenderSpheres()
    {
        Profiler.BeginSample("Render spheres");
        if (_sphereCount > 0)
        {
            var props = new MaterialPropertyBlock();
            props.SetVectorArray("_Color", _sphereColors);
            Graphics.DrawMeshInstanced(sphereMesh, 0, gridMaterial, _sphereMatrices, _sphereCount, props);
            _sphereCount = 0;
        }
        Profiler.EndSample();
    }

    public static void DrawSphere(Vector3 position, float radius, Color color)
    {
        _sphereMatrices[_sphereCount] = Matrix4x4.Translate(position) * Matrix4x4.Scale(new Vector3(radius, radius, radius));
        _sphereColors[_sphereCount] = new Vector4(color.r, color.g, color.b, color.a);
        _sphereCount++;
        if (_sphereCount >= 1022)
        {
            RenderSpheres();
        }
    }

    static void RenderCones()
    {
        Profiler.BeginSample("Render cones");
        if (_coneCount > 0)
        {
            var props = new MaterialPropertyBlock();
            props.SetVectorArray("_Color", _coneColors);
            Graphics.DrawMeshInstanced(coneMesh, 0, gridMaterial, _coneMatrices, _coneCount, props);
            _coneCount = 0;
        }
        Profiler.EndSample();
    }

    public static void DrawCone(Vector3 position, Vector3 tipDirection, float size, Color color)
    {
        Matrix4x4 rot = Matrix4x4.Rotate(Quaternion.FromToRotation(Vector3.up, tipDirection.normalized));
        _coneMatrices[_coneCount] = Matrix4x4.Translate(position) * rot * Matrix4x4.Scale(new Vector3(size, size, size));
        _coneColors[_coneCount] = new Vector4(color.r, color.g, color.b, color.a);
        _coneCount++;
        if (_coneCount >= 1022)
        {
            RenderCones();
        }
    }

    static void RenderCubes()
    {
        Profiler.BeginSample("Render cubes");
        if (_cubeCount > 0)
        {
            var props = new MaterialPropertyBlock();
            props.SetVectorArray("_Color", _cubeColors);
            Graphics.DrawMeshInstanced(cubeMesh, 0, gridMaterial, _cubeMatrices, _cubeCount, props);
            _cubeCount = 0;
        }
        Profiler.EndSample();
    }

    public static void DrawCube(Vector3 position, float size, Color color)
    {
        _cubeMatrices[_cubeCount] = Matrix4x4.Translate(position) * Matrix4x4.Scale(new Vector3(size, size, size));
        _cubeColors[_cubeCount] = color;
        _cubeCount++;

        if(_cubeCount >= 1022)
        {
            RenderCubes();
        }
    }

    public static void DrawCubeEdges(Vector3 position, float size, float lineRadius, Color color)
    {
        Vector3 minCorner = position - new Vector3(size / 2.0f, size / 2.0f, size / 2.0f);
        Vector3 x = minCorner + new Vector3(size, 0.0f, 0.0f);
        Vector3 y = minCorner + new Vector3(0.0f, size, 0.0f);
        Vector3 z = minCorner + new Vector3(0.0f, 0.0f, size);
        Vector3 xy = minCorner + new Vector3(size, size, 0.0f);
        Vector3 xz = minCorner + new Vector3(size, 0.0f, size);
        Vector3 yz = minCorner + new Vector3(0.0f, size, size);
        Vector3 xyz = minCorner + new Vector3(size, size, size);
        DrawLine(minCorner, x, lineRadius, color);
        DrawLine(minCorner, y, lineRadius, color);
        DrawLine(minCorner, z, lineRadius, color);
        DrawLine(y, xy, lineRadius, color);
        DrawLine(x, xy, lineRadius, color);
        DrawLine(x, xz, lineRadius, color);
        DrawLine(xy, xyz, lineRadius, color);
        DrawLine(y, yz, lineRadius, color);
        DrawLine(yz, xyz, lineRadius, color);
        DrawLine(xyz, xz, lineRadius, color);
        DrawLine(z, xz, lineRadius, color);
        DrawLine(z, yz, lineRadius, color);

    }

    public static void Draw3DArrow(Vector3 start, Vector3 end, float size, Color color, Color? tipColor = null)
    {
        const float tipSize = 3.5f;
        Vector3 dir = (end-start).normalized;
        DrawLine(start, end - dir*size*tipSize, size, color);
        DrawCone(end - dir*size*tipSize, dir, size*tipSize, tipColor == null ? color : tipColor.Value);
    }

    public static void DrawGrid2D(Vector3 minCorner, float stepSize, float length, float radius, Color color)
    {
        if (cylinderMesh == null)
            Debug.Log("Error");
        var props = new MaterialPropertyBlock();
        for (int i = 0; i <= length / stepSize; i++)
        {
            var mat = Matrix4x4.TRS(
                    minCorner + new Vector3(i * stepSize, 0.0f, length / 2.0f), 
                    Quaternion.AngleAxis(90.0f, Vector3.right), 
                    new Vector3(radius, length / 2.0f, radius)
                );
            props.SetColor("_Color", color);
            Graphics.DrawMesh(cylinderMesh, mat, gridMaterial, 0, Camera.main, 0, props);
        }
        for (int i = 0; i <= length / stepSize; i++)
        {
            var mat = Matrix4x4.TRS(
                    minCorner + new Vector3(length / 2.0f, 0.0f, i * stepSize), 
                    Quaternion.AngleAxis(90.0f, Vector3.forward), 
                    new Vector3(radius, length / 2.0f, radius)
            );
            props.SetColor("_Color", color);
            Graphics.DrawMesh(cylinderMesh, mat, gridMaterial, 0, Camera.main, 0, props);
        }
    }

    public static void DrawGrid3D(Vector3 minCorner, float stepSize, int steps, float radius, Color color)
    {
        float length = stepSize * steps;

        for (int j = 0; j <= steps; j++)
        {
            for (int i = 0; i <= steps; i++)
            {
                var start = minCorner + new Vector3(i * stepSize, j * stepSize, 0.0f);
                DrawLine(start, start + new Vector3(0.0f, 0.0f, length), radius, color);
            }
            for (int i = 0; i <= steps; i++)
            {
                var start = minCorner + new Vector3(0.0f, j * stepSize, i * stepSize);
                DrawLine(start, start + new Vector3(length, 0.0f, 0.0f), radius, color);
            }
        }
        for (int j = 0; j <= steps; j++)
        {
            for (int i = 0; i <= steps; i++)
            {
                var start = minCorner + new Vector3(j * stepSize, 0.0f, i * stepSize);
                DrawLine(start, start + new Vector3(0.0f, length, 0.0f), radius, color);
            }
        }
    }


    public static void DrawVertices(List<Vector3> vertices, Color color)
    {
        var mesh = new Mesh();
        mesh.SetVertices(vertices);
        int[] indices = new int[vertices.Count];
        for (int i = 0; i < indices.Length; i++)
            indices[i] = i;
        mesh.triangles = indices;
        mesh.RecalculateBounds();
        mesh.RecalculateNormals();
        MaterialPropertyBlock props = new MaterialPropertyBlock();
        props.SetColor("_Color", color);
        Graphics.DrawMesh(mesh, Matrix4x4.identity, cubeMaterial, 0, Camera.main, 0, props);
        _deleteMeshes.Add(mesh);
    }

    public static void DrawTriangleEdges(List<Vector3> vertices, float lineSize, Color color)
    {
        for (int i = 0; i <= vertices.Count - 3; i += 3)
        {
            DrawLine(vertices[i], vertices[i + 1], lineSize, color);
            DrawLine(vertices[i], vertices[i + 2], lineSize, color);
            DrawLine(vertices[i + 1], vertices[i + 2], lineSize, color);
        }
        int mod = vertices.Count % 3;
        if (mod != 0)
        {
            if (mod == 2)
                DrawLine(vertices[vertices.Count - 2], vertices[vertices.Count - 1], lineSize, color);
        }
    }

    public static void DrawTriangle(Vector3 vert1, Vector3 vert2, Vector3 vert3, Color color)
    {
        // TODO: color
        tempVertices.Add(vert1);
        tempVertices.Add(vert2);
        tempVertices.Add(vert3);
        tempVertexColors.Add(color);
        tempVertexColors.Add(color);
        tempVertexColors.Add(color);
    }

    static readonly Vector3[] cubeVertices = new Vector3[]
    {
        new Vector3(-0.5f, -0.5f,  0.5f ),
        new Vector3( 0.5f, -0.5f,  0.5f ),
        new Vector3( 0.5f,  0.5f,  0.5f ),
        new Vector3(-0.5f,  0.5f,  0.5f ),
        new Vector3( 0.5f, -0.5f,  0.5f ),
        new Vector3( 0.5f, -0.5f, -0.5f ),
        new Vector3( 0.5f,  0.5f, -0.5f ),
        new Vector3( 0.5f,  0.5f,  0.5f ),
        new Vector3( 0.5f, -0.5f, -0.5f ),
        new Vector3(-0.5f, -0.5f, -0.5f ),
        new Vector3(-0.5f,  0.5f, -0.5f ),
        new Vector3( 0.5f,  0.5f, -0.5f ),
        new Vector3(-0.5f, -0.5f, -0.5f ),
        new Vector3(-0.5f, -0.5f,  0.5f ),
        new Vector3(-0.5f,  0.5f,  0.5f ),
        new Vector3(-0.5f,  0.5f, -0.5f ),
        new Vector3(-0.5f,  0.5f,  0.5f ),
        new Vector3( 0.5f,  0.5f,  0.5f ),
        new Vector3( 0.5f,  0.5f, -0.5f ),
        new Vector3(-0.5f,  0.5f, -0.5f ),
        new Vector3(-0.5f, -0.5f, -0.5f ),
        new Vector3( 0.5f, -0.5f, -0.5f ),
        new Vector3( 0.5f, -0.5f,  0.5f ),
        new Vector3(-0.5f, -0.5f,  0.5f )
    };

    static readonly int[] cubeIndices = new int[]
    {
        0,   1,  2,  2,  3,  0,
        4,   5,  6,  6,  7,  4,
        8,   9, 10, 10, 11,  8,
        12, 13, 14, 14, 15, 12,
        16, 17, 18, 18, 19, 16,
        20, 21, 22, 22, 23, 20
    };

    static readonly Vector3[] coneVertices = new Vector3[]
    {
        new Vector3(0.000000f, -1.000000f, -1.000000f),
        new Vector3(0.000000f, 1.000000f, 0.000000f),
        new Vector3(0.195090f, -1.000000f, -0.980785f),
        new Vector3(0.382683f, -1.000000f, -0.923880f),
        new Vector3(0.555570f, -1.000000f, -0.831470f),
        new Vector3(0.707107f, -1.000000f, -0.707107f),
        new Vector3(0.831470f, -1.000000f, -0.555570f),
        new Vector3(0.923880f, -1.000000f, -0.382683f),
        new Vector3(0.980785f, -1.000000f, -0.195090f),
        new Vector3(1.000000f, -1.000000f, -0.000000f),
        new Vector3(0.980785f, -1.000000f, 0.195090f),
        new Vector3(0.923880f, -1.000000f, 0.382683f),
        new Vector3(0.831470f, -1.000000f, 0.555570f),
        new Vector3(0.707107f, -1.000000f, 0.707107f),
        new Vector3(0.555570f, -1.000000f, 0.831470f),
        new Vector3(0.382683f, -1.000000f, 0.923880f),
        new Vector3(0.195090f, -1.000000f, 0.980785f),
        new Vector3(-0.000000f, -1.000000f, 1.000000f),
        new Vector3(-0.195091f, -1.000000f, 0.980785f),
        new Vector3(-0.382684f, -1.000000f, 0.923879f),
        new Vector3(-0.555571f, -1.000000f, 0.831469f),
        new Vector3(-0.707107f, -1.000000f, 0.707106f),
        new Vector3(-0.831470f, -1.000000f, 0.555570f),
        new Vector3(-0.923880f, -1.000000f, 0.382683f),
        new Vector3(-0.980785f, -1.000000f, 0.195089f),
        new Vector3(-1.000000f, -1.000000f, -0.000001f),
        new Vector3(-0.980785f, -1.000000f, -0.195091f),
        new Vector3(-0.923879f, -1.000000f, -0.382684f),
        new Vector3(-0.831469f, -1.000000f, -0.555571f),
        new Vector3(-0.707106f, -1.000000f, -0.707108f),
        new Vector3(-0.555569f, -1.000000f, -0.831470f),
        new Vector3(-0.382682f, -1.000000f, -0.923880f),
        new Vector3(-0.195089f, -1.000000f, -0.980786f),
        new Vector3(0.000000f, -1.000000f, -1.000000f),
        new Vector3(0.195090f, -1.000000f, -0.980785f),
        new Vector3(0.382683f, -1.000000f, -0.923880f),
        new Vector3(0.555570f, -1.000000f, -0.831470f),
        new Vector3(0.707107f, -1.000000f, -0.707107f),
        new Vector3(0.831470f, -1.000000f, -0.555570f),
        new Vector3(0.923880f, -1.000000f, -0.382683f),
        new Vector3(0.980785f, -1.000000f, -0.195090f),
        new Vector3(1.000000f, -1.000000f, -0.000000f),
        new Vector3(0.980785f, -1.000000f, 0.195090f),
        new Vector3(0.923880f, -1.000000f, 0.382683f),
        new Vector3(0.831470f, -1.000000f, 0.555570f),
        new Vector3(0.707107f, -1.000000f, 0.707107f),
        new Vector3(0.555570f, -1.000000f, 0.831470f),
        new Vector3(0.382683f, -1.000000f, 0.923880f),
        new Vector3(0.195090f, -1.000000f, 0.980785f),
        new Vector3(-0.000000f, -1.000000f, 1.000000f),
        new Vector3(-0.195091f, -1.000000f, 0.980785f),
        new Vector3(-0.382684f, -1.000000f, 0.923879f),
        new Vector3(-0.555571f, -1.000000f, 0.831469f),
        new Vector3(-0.707107f, -1.000000f, 0.707106f),
        new Vector3(-0.831470f, -1.000000f, 0.555570f),
        new Vector3(-0.923880f, -1.000000f, 0.382683f),
        new Vector3(-0.980785f, -1.000000f, 0.195089f),
        new Vector3(-1.000000f, -1.000000f, -0.000001f),
        new Vector3(-0.980785f, -1.000000f, -0.195091f),
        new Vector3(-0.923879f, -1.000000f, -0.382684f),
        new Vector3(-0.831469f, -1.000000f, -0.555571f),
        new Vector3(-0.707106f, -1.000000f, -0.707108f),
        new Vector3(-0.555569f, -1.000000f, -0.831470f),
        new Vector3(-0.382682f, -1.000000f, -0.923880f),
        new Vector3(-0.195089f, -1.000000f, -0.980786f),    
    };

    static readonly int[] coneIndices = new int[]
    {
        31, 1, 32, 0, 1, 2, 30, 1, 31, 29, 1, 30, 28, 
        1, 29, 27, 1, 28, 26, 1, 27, 25, 1, 26, 24, 
        1, 25, 23, 1, 24, 22, 1, 23, 21, 1, 22, 20, 
        1, 21, 19, 1, 20, 18, 1, 19, 17, 1, 18, 16, 
        1, 17, 15, 1, 16, 14, 1, 15, 13, 1, 14, 12, 
        1, 13, 11, 1, 12, 10, 1, 11, 9, 1, 10, 8, 
        1, 9, 7, 1, 8, 6, 1, 7, 5, 1, 6, 4, 
        1, 5, 3, 1, 4, 32, 1, 0, 2, 1, 3, 33, 
        34, 64, 34, 35, 64, 35, 36, 64, 36, 37, 64, 37, 
        38, 64, 38, 39, 64, 39, 40, 64, 40, 41, 64, 41, 
        42, 64, 42, 43, 64, 43, 44, 64, 44, 45, 64, 59, 
        57, 58, 45, 46, 47, 64, 45, 47, 50, 48, 49, 50, 
        47, 48, 64, 47, 50, 53, 51, 52, 53, 50, 51, 64, 
        50, 53, 56, 54, 55, 63, 64, 53, 59, 56, 57, 59, 
        54, 56, 62, 63, 53, 61, 62, 53, 60, 61, 53, 59, 
        60, 54, 60, 53, 54
    };

}
