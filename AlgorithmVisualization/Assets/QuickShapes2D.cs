using System.Collections;
using System.Collections.Generic;
using System.Linq;
using UnityEngine;
using UnityEngine.UI;

[ExecuteInEditMode]
public class QuickShapes2D : MonoBehaviour 
{
    public Material material;
    public Material lineMaterial;
    public Texture texture;
    public float blurAmount = 0.99f;

    List<Vector3> _circleVertices;
    List<Color> _circleColors;
    Mesh _circleMesh;

    List<Vector3> _lineVertices;
    List<Color> _lineColors;
    Mesh _lineMesh;

    static QuickShapes2D _instance;
    static QuickShapes2D instance
    {
        get
        {
            if(_instance == null)
                _instance = FindObjectOfType<QuickShapes2D>();
            return _instance;
        }

        set
        {
            if(_instance != null && value != _instance)
                Destroy(_instance);
            _instance = value;
        }
    }

    void OnEnable()
    {
        instance = this;
        if(material == null)
            material = new Material(Shader.Find("Standard"));
        if(lineMaterial == null)
            lineMaterial = new Material(Shader.Find("Standard"));
    
        _circleVertices = new List<Vector3>();
        _circleColors = new List<Color>();
        _circleMesh = new Mesh();

        _lineVertices = new List<Vector3>();
        _lineColors = new List<Color>();
        _lineMesh = new Mesh();

    }

    void OnDisable()
    {
        _circleVertices = null;
        _circleColors = null;
        _circleMesh = null;

        _lineVertices = null;
        _lineColors = null;
        _lineMesh = null;
    }

	// Use this for initialization
	void Start () {
	    /*Mesh mesh = new Mesh();
        float size = 1000.0f;
        mesh.vertices = new Vector3[] { 
            new Vector3(0.0f, 0.0f, size),
            new Vector3(0.0f, size, size),
            new Vector3(size, size, size),
            new Vector3(size, 0.0f, size)
        };
        mesh.uv = new Vector2[] {
            new Vector2(0.0f, 0.0f),
            new Vector2(0.0f, 1.0f),
            new Vector2(1.0f, 1.0f),
            new Vector2(1.0f, 0.0f)
        };
        var colors = new Color[] {
            new Color(0.99f, 0.0f, 0.0f, 0.95f),
            new Color(0.99f, 0.0f, 0.0f, 0.95f),
            new Color(0.99f, 0.0f, 0.0f, 0.95f),
            new Color(0.99f, 0.0f, 0.0f, 0.95f)
        };
        mesh.triangles = new int[] {
            0, 1, 2, 2, 3, 0
        };
        mesh.colors = colors;
        canvasRenderer.SetMaterial(material, texture);
        canvasRenderer.SetMesh(mesh);
        */
	}

    public static void DrawCircle(Vector2 position, float size, Color color)
    {
        float hsize = size*0.5f;
        instance._circleVertices.Add(new Vector3(position.x-hsize, position.y-hsize, hsize));
        instance._circleVertices.Add(new Vector3(position.x-hsize, position.y+hsize, hsize));
        instance._circleVertices.Add(new Vector3(position.x+hsize, position.y+hsize, hsize));
        instance._circleVertices.Add(new Vector3(position.x+hsize, position.y-hsize, hsize));
        instance._circleColors.Add(new Color(color.r, color.g, color.b, _instance.blurAmount));
        instance._circleColors.Add(new Color(color.r, color.g, color.b, _instance.blurAmount));
        instance._circleColors.Add(new Color(color.r, color.g, color.b, _instance.blurAmount));
        instance._circleColors.Add(new Color(color.r, color.g, color.b, _instance.blurAmount));
    }

    public static void DrawLine(Vector2 start, Vector2 end, float size, Color color)
    {
        instance._lineVertices.Add(new Vector3(start.x, start.y, size));
        instance._lineVertices.Add(new Vector3(end.x, end.y, size));
        instance._lineColors.Add(color);
    }

    public static void DrawPath(List<Vector3> path, float size, Color color)
    {
        for (int i = 0; i < path.Count() - 1; i++)
        {
            DrawLine(path[i], path[i + 1], size, color);
        }
    }

    static void RenderCircles()
    {
        if(instance._circleVertices.Count <= 0)
        {
            return;
        }
        
        Vector2[] uvs = new Vector2[instance._circleVertices.Count];
        int[] tris = new int[(instance._circleVertices.Count/4)*6];
        for(int i = 0, ti = 0; i < instance._circleVertices.Count; i+=4)
        {
            uvs[i+0] = new Vector2(0.0f, 0.0f);
            uvs[i+1] = new Vector2(0.0f, 1.0f);
            uvs[i+2] = new Vector2(1.0f, 1.0f);
            uvs[i+3] = new Vector2(1.0f, 0.0f);
            tris[ti++] = i+0;
            tris[ti++] = i+1;
            tris[ti++] = i+2;
            tris[ti++] = i+2;
            tris[ti++] = i+3;
            tris[ti++] = i+0;
        }


        instance._circleMesh.SetVertices(instance._circleVertices);
        instance._circleMesh.uv = uvs;
        instance._circleMesh.SetColors(instance._circleColors);
        instance._circleMesh.triangles = tris;

        instance._circleVertices.Clear();
        instance._circleColors.Clear();
        //instance._circleMesh.Clear();
        
        Graphics.DrawMesh(instance._circleMesh, instance.transform.localToWorldMatrix, instance.material, 0);
    }

    static void RenderLines()
    {
        if(instance._lineVertices.Count <= 0)
            return;
        var varr = instance._lineVertices;
        var carr = instance._lineColors;
        Debug.Assert((varr.Count&1) == 0);
        int lineCount = varr.Count / 2;
        Vector3[] vertices = new Vector3[lineCount * 4];
        Vector2[] uvs = new Vector2[lineCount * 4];
        Color[] colors = new Color[lineCount * 4];
        int[] indices = new int[lineCount*6];

        for(int i = 0; i < lineCount; i++)
        {
            int i4 = 4 * i;
            float size = varr[i*2].z;
            float hsize = 0.5f*size;
            Vector2 start = varr[i*2];
            Vector2 dest = varr[i*2 + 1];
            Vector2 sToD = dest - start;
            Vector2 pepLHS = new Vector2(sToD.y, -sToD.x).normalized;
            
            vertices[i4 + 0] = start + hsize*pepLHS;
            vertices[i4 + 1] = start - hsize*pepLHS;
            vertices[i4 + 2] = dest + hsize*pepLHS;
            vertices[i4 + 3] = dest - hsize*pepLHS;
            for(int j = 0; j < 4; j++)
                colors[i4 + j] = carr[i];

            indices[i*6 + 0] = i4 + 0;
            indices[i*6 + 1] = i4 + 3;
            indices[i*6 + 2] = i4 + 2;

            indices[i*6 + 3] = i4 + 0;
            indices[i*6 + 4] = i4 + 1;
            indices[i*6 + 5] = i4 + 3;
        }

        instance._lineMesh.Clear();
        instance._lineMesh.vertices = vertices;
        instance._lineMesh.triangles = indices;
        instance._lineMesh.colors = colors;

        instance._lineVertices.Clear();
        instance._lineColors.Clear();

        Graphics.DrawMesh(instance._lineMesh, instance.transform.localToWorldMatrix, instance.lineMaterial, 0);
    }
	
	// Update is called once per frame
	void LateUpdate () {
		
        RenderCircles();
        RenderLines();
	}
}
