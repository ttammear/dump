using System.Collections;
using System.Collections.Generic;
using System.Linq;
using UnityEngine;
using UnityEngine.Profiling;

[DefaultExecutionOrder(-1000)]
public class QuickShapes : MonoBehaviour {

  static Mesh cubeMesh;
  static Mesh cylinderMesh;
  static Material cubeMaterial;
  static Material gridMaterial;

  struct RenderCube
  {
    Vector3 position;
    Color color;
  }

  public Material CubeMaterial;
  public Material GridMaterial;
  List<RenderCube> _cubes = new List<RenderCube>();

  private void Awake()
  {
    cubeMesh = new Mesh();
    cubeMesh.vertices = cubeVertices;
    cubeMesh.triangles = cubeIndices;
    cubeMesh.RecalculateNormals();
    cubeMesh.RecalculateBounds();

    cubeMaterial = CubeMaterial;
    gridMaterial = GridMaterial;

    GameObject go = GameObject.CreatePrimitive(PrimitiveType.Cylinder);
    cylinderMesh = go.GetComponent<MeshFilter>().sharedMesh;
    Destroy(go);
  }
  // Use this for initialization
  void Start () {
		
	}
	
	// Update is called once per frame
	void Update () {
    _cubes.Clear();
    _lineColors.Clear();
    _lineCount = 0;
    foreach (var mesh in _deleteMeshes)
      MonoBehaviour.Destroy(mesh);
	}

  private void LateUpdate()
  {
    RenderLines();
  }

  static Vector3[] cubeVertices = new Vector3[]
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

  static int[] cubeIndices = new int[]
  {
    0, 1, 2, 2, 3, 0,
    4, 5, 6, 6, 7, 4,
    8, 9, 10, 10, 11, 8,
    12, 13, 14, 14, 15, 12,
    16, 17, 18, 18, 19, 16,
    20, 21, 22, 22, 23, 20
  };

  static Matrix4x4[] _lineMatrices = new Matrix4x4[1024];
  static List<Vector4> _lineColors = new List<Vector4>();
  static int _lineCount = 0;

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
    _lineMatrices[_lineCount] = Matrix4x4.TRS(start+(end-start)*0.5f, Quaternion.LookRotation((start-end).normalized)*Quaternion.AngleAxis(90.0f, Vector3.right), new Vector3(radius, length / 2.0f, radius));
    _lineColors.Add(new Vector4(color.r, color.g, color.b, color.a));
    _lineCount++;
    if (_lineCount >= 1023)
    {
      RenderLines();
    }
  }

  public static void DrawCube(Vector3 position, float size, Color color)
  {
    var mat = Matrix4x4.Translate(position);
    var props = new MaterialPropertyBlock();
    props.SetColor("_Color", color);
    Graphics.DrawMesh(cubeMesh, mat, cubeMaterial, 0, Camera.main, 0, props);
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

  public static void DrawGrid2D(Vector3 minCorner, float stepSize, float length, float radius, Color color)
  {
    if (cylinderMesh == null)
      Debug.Log("Error");
    var props = new MaterialPropertyBlock();
    for (int i = 0; i <= length / stepSize; i++)
    {
      var mat = Matrix4x4.TRS(minCorner + new Vector3(i * stepSize, 0.0f, length/2.0f), Quaternion.AngleAxis(90.0f, Vector3.right), new Vector3(radius, length/2.0f, radius));
      //var mat = Matrix4x4.Translate(minCorner);
      props.SetColor("_Color", color);
      Graphics.DrawMesh(cylinderMesh, mat, gridMaterial, 0, Camera.main, 0, props);
    }
    for (int i = 0; i <= length / stepSize; i++)
    {
      var mat = Matrix4x4.TRS(minCorner + new Vector3(length / 2.0f, 0.0f, i*stepSize), Quaternion.AngleAxis(90.0f, Vector3.forward), new Vector3(radius, length/2.0f, radius));
      //var mat = Matrix4x4.Translate(minCorner);
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
        var start = minCorner + new Vector3(0.0f, j * stepSize, i*stepSize);
        DrawLine(start, start + new Vector3(length, 0.0f, 0.0f), radius, color);
      }
    }
    for (int j = 0; j <= steps; j++)
    {
      for (int i = 0; i <= steps; i++)
      {
        var start = minCorner + new Vector3(j * stepSize, 0.0f, i*stepSize);
        DrawLine(start, start + new Vector3(0.0f, length, 0.0f), radius, color);
      }
    }
  }

  static List<Mesh> _deleteMeshes = new List<Mesh>();

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
    for(int i = 0; i < vertices.Count; i+=3)
    {
      DrawLine(vertices[i], vertices[i + 1], lineSize, color);
      DrawLine(vertices[i], vertices[i + 2], lineSize, color);
      DrawLine(vertices[i+1], vertices[i + 2], lineSize, color);
    }
  }
}
