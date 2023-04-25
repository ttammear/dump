using System;
using UnityEngine;
using System.Collections;
using System.Collections.Generic;

public class DualCAnimation : MonoBehaviour
{
    struct PointNormal
    {
        public Vector3 pos;
        public Vector3 normal;
    }

    struct Line
    {
        public Vector3 start;
        public Vector3 end;
        public Color color;
    }

    DCOctree dc;
    MCubesRef mcubes;
    public Material fadeMaterial;
    public GameObject torusGo;
    public Sprite formulaImg;

    List<PointNormal> pnormals = new List<PointNormal>();
    List<Line> lines = new List<Line>();

    List<Line> polyLines = new List<Line>();

    bool drawLine1 = false;
    Line line1;

    Vector3 qefR1;
    Vector3 qefR1MP;
    bool showQefR1 = false;

    bool showGrid = true;

    float sdTorus(Vector3 p/*, vec2 t */)
    {
        Vector2 t = new Vector2(5.0f, 2.0f);
        float pxzm = new Vector2(p.x, p.z).magnitude;
        Vector2 q = new Vector2(pxzm-t.x, p.y);
        return q.magnitude - t.y;
    }

    Vector3 sdTorusNormal(Vector3 p)
    {
        float R = 5.0f;
        Vector2 c = Vector2.zero;

        Vector2 v = new Vector2(p.x, p.z);
        float magV = v.magnitude;
        Vector2 a = c + v / magV * R;

        //return (p - a).normalized;
        return (p - new Vector3(a.x, 0.0f, a.y)).normalized;
    }

    void Start()
    {
        Debug.Log("Fuck this shieeet");
        dc = new DCOctree(4, Vector3.zero);
        dc.DebugRender(dc.rootNode);
        mcubes = new MCubesRef();

        //QuickLabels.ShowLabel2(new Vector2(100.0f, 50.0f), "Fucking text", 30, Color.red);
        int size = 32;
        float scale = 0.5f;
        float extent = size * 0.5f * scale;
        Vector3 offset = new Vector3(-extent, -extent, -extent);

        var torusMesh = mcubes.GetMesh(offset, size, scale);
        torusGo = new GameObject();
        var torusMF = torusGo.AddComponent<MeshFilter>();
        var torusMR = torusGo.AddComponent<MeshRenderer>();
        torusMF.sharedMesh = torusMesh;
        torusMR.sharedMaterial = new Material(fadeMaterial);
        Color tcol = new Color(0.9f, 0.5f, 0.0f, 1.0f);
        tcol.a = 0.1f;
        torusMR.sharedMaterial.color = tcol;

        transform.position = new Vector3(-8.30f, 6.43f, -8.58f);
        transform.rotation = Quaternion.Euler(33.8f, 46.4f, 0.0f);

        StartCoroutine(PlayAnimation());
    }

    IEnumerator DrawCubeIntersections(Vector3 pos, float size)
    {
        QefSolver qef = new QefSolver();

        int cornerMask = 0;
        for(int i = 0; i < 8; i++)
        {
            if(sdTorus(pos + DCOctree.childOffsets[i]*size) < 0.0f)
                cornerMask |= 1 << i;
        }

        int crossings = 0;
        Vector3 avgPos = Vector3.zero;
        for(int i = 0; i < 12 && crossings < 6; i++)
        {
            int ofstI1 = DCOctree.edgevmap[i, 0];
            int ofstI2 = DCOctree.edgevmap[i, 1];
            
            Vector3 ofst1 = pos + DCOctree.childOffsets[ofstI1];
            Vector3 ofst2 = pos + DCOctree.childOffsets[ofstI2];

            line1.start = ofst1;
            line1.end = ofst2;
            drawLine1 = true;

            if(((cornerMask >> ofstI1) & 1) != ((cornerMask >> ofstI2) & 1) && crossings < 6)
            {
                Vector3 ipoint = DCOctree.FindIntersection(ofst1, ofst2);
                Vector3 inormal = sdTorusNormal(ipoint);
                pnormals.Add(new PointNormal() { pos = ipoint, normal = inormal } );
                line1.color = Color.green;
                qef.Add(ipoint.x, ipoint.y, ipoint.z, inormal.x, inormal.y, inormal.z);
                avgPos += ipoint;
                crossings++;
            }
            else
            {
                line1.color = Color.red;
            }
            yield return new CustomWait(0.8f);
        }

        Vector3 qefPos;
        qef.Solve(out qefPos, 1e-6f, 4, 1e-6f);
        qefR1 = qefPos;
        qefR1MP = avgPos / crossings;

        drawLine1 = false;
        yield return null;
    }

    int textSizeMult = 1;

    IEnumerator PlayAnimation()
    {
        var sw = new System.Diagnostics.Stopwatch();
        sw.Start();

        yield return MoveRotateOperation.MoveRotate(transform, new Vector3(-4.27f, 0.67f, -6.76f), Quaternion.Euler(8.6f, 67.9f, 0.0f), 3.0f); 

        int image;
        //torusGo.SetActive(false);

        Color bgColor = new Color(0.0f, 0.0f, 0.0f, 0.5f);


        yield return new CustomWait(1.0f);

        int label;
        label = QuickLabels.ShowLabelWithBackground(new Rect(100.0f * textSizeMult, 400.0f * textSizeMult, 1000.0f * textSizeMult, 200.0f * textSizeMult), "First cell edge intersection points and normals are found.\nIf no intersections were found, then that cell won't contain a vertex.", textSizeMult * 32, Color.white, bgColor);
        
        yield return new CustomWait(1.0f);

        yield return DrawCubeIntersections(new Vector3(x - 8.0f, 0.0f, z - 8.0f), 1.0f);

        yield return new CustomWait(4.0f);
        
        QuickLabels.HideLabelWithBackground(label);
        label = QuickLabels.ShowLabelWithBackground(new Rect(100.0f * textSizeMult, 400.0f * textSizeMult, 1000.0f * textSizeMult, 200.0f * textSizeMult), "Otherwise the points and normals are used for a QEF (Quadratic Error Function). QEF finds the minimizer of the function (vertex):", textSizeMult * 32, Color.white, bgColor);
        image = QuickLabels.ShowImage(new Vector2(100.0f * textSizeMult, 500.0f * textSizeMult), formulaImg, 300.0f * textSizeMult); 

        yield return new CustomWait(4.0f);

        showQefR1 = true;
        int qr1l = QuickLabels.Show3DLabel(qefR1, "vertex", 30 * textSizeMult, Color.black, new Vector2(50.0f * textSizeMult, 30.0f * textSizeMult));
        //int qr1mpl = QuickLabels.Show3DLabel(qefR1MP, "mass point", 30 * textSizeMult, Color.black, new Vector2(-85.0f * textSizeMult, -30.0f * textSizeMult));

        yield return new CustomWait(4.0f);
        QuickLabels.HideImage(image);
        QuickLabels.HideLabelWithBackground(label);
        QuickLabels.Hide3DLabel(qr1l);
        //QuickLabels.Hide3DLabel(qr1mpl);
    
        yield return MoveRotateOperation.MoveRotate(transform, new Vector3(-2.08f, 6.25f, -9.93f), Quaternion.Euler(39.58f, 8.0f, 0.0f), 3.0f); 

        yield return new CustomWait(0.8f);
        //torusGo.SetActive(false);
        showQefR1 = false;
        showGrid = false;
        pnormals.Clear();
        //showCellEdges = false;

        for(; showVertsCount < dc._animVertices.Count; showVertsCount++)
        {
            showVerts = true;
            if(showVertsCount > 0)
            {
                x = Mathf.FloorToInt(dc._animVertices[showVertsCount-1].x) + 8;
                y = Mathf.FloorToInt(dc._animVertices[showVertsCount-1].y) + 8;
                z = Mathf.FloorToInt(dc._animVertices[showVertsCount-1].z) + 8;
            }
            yield return new CustomWait(1); // 0.05f
        }
        showCellEdges = false;

        yield return new CustomWait(1.0f);


        label = QuickLabels.ShowLabelWithBackground(new Rect(100.0f * textSizeMult, 400.0f * textSizeMult, 1000.0f * textSizeMult, 200.0f * textSizeMult), "Finally a quad is generated at each intersecting cell edge.\nThe quad is generated from 4 cells surrounding the intersecting edge.", textSizeMult * 32, Color.white, bgColor);

        showIntersectingEdges = true;

        yield return new CustomWait(10.0f);
        showGrid = true;

        QuickLabels.HideLabelWithBackground(label);

        yield return MoveRotateOperation.MoveRotate(transform, new Vector3(-3.44f, 2.02f, -7.43f), Quaternion.Euler(31.0f, 58.0f, 0.0f), 3.0f); 

        yield return new CustomWait(1.0f);

        drawPolyLines = true;

        var intEdge = dc._intEdges[104];
        curtest = 104;
        polyLines.Add(new Line() { start = intEdge.node1.intersectionPoint, end = intEdge.node2.intersectionPoint});
        polyLines.Add(new Line() { start = intEdge.node2.intersectionPoint, end = intEdge.node3.intersectionPoint});
        polyLines.Add(new Line() { start = intEdge.node3.intersectionPoint, end = intEdge.node1.intersectionPoint});
        polyLines.Add(new Line() { start = intEdge.node2.intersectionPoint, end = intEdge.node4.intersectionPoint});
        polyLines.Add(new Line() { start = intEdge.node4.intersectionPoint, end = intEdge.node3.intersectionPoint});
        polyLines.Add(new Line() { start = intEdge.node3.intersectionPoint, end = intEdge.node2.intersectionPoint});
        s4Cells = new DCOctree.DCNode[] {intEdge.node1, intEdge.node2, intEdge.node3, intEdge.node4};
        show4Cells = true;

        yield return new CustomWait(3.0f);
        
        yield return MoveRotateOperation.MoveRotate(transform, new Vector3(-4.10f, 3.94f, -7.43f), Quaternion.Euler(42.1f, 54.43f, 0.0f), 3.0f); 
        yield return new CustomWait(1.0f);

        intEdge = dc._intEdges[109];
        curtest = 109;
        polyLines.Add(new Line() { start = intEdge.node1.intersectionPoint, end = intEdge.node2.intersectionPoint});
        polyLines.Add(new Line() { start = intEdge.node2.intersectionPoint, end = intEdge.node3.intersectionPoint});
        polyLines.Add(new Line() { start = intEdge.node3.intersectionPoint, end = intEdge.node1.intersectionPoint});
        polyLines.Add(new Line() { start = intEdge.node2.intersectionPoint, end = intEdge.node4.intersectionPoint});
        polyLines.Add(new Line() { start = intEdge.node4.intersectionPoint, end = intEdge.node3.intersectionPoint});
        polyLines.Add(new Line() { start = intEdge.node3.intersectionPoint, end = intEdge.node2.intersectionPoint});
        s4Cells = new DCOctree.DCNode[] {intEdge.node1, intEdge.node2, intEdge.node3, intEdge.node4};
        show4Cells = true;

        yield return new CustomWait(3.0f);

        yield return MoveRotateOperation.MoveRotate(transform, new Vector3(-5.87f, 7.52f, -9.64f), Quaternion.Euler(36.88f, 32.1f, 0.0f), 3.0f); 

        yield return new CustomWait(1.0f);

        show4Cells = false;
        torusGo.SetActive(false);
        showGrid = false;

        for(int i = 0; i < dc._intEdges.Count; i++)
        {
            intEdge = dc._intEdges[i];
            curtest = i;
            polyLines.Add(new Line() { start = intEdge.node1.intersectionPoint, end = intEdge.node2.intersectionPoint});
            polyLines.Add(new Line() { start = intEdge.node2.intersectionPoint, end = intEdge.node3.intersectionPoint});
            polyLines.Add(new Line() { start = intEdge.node3.intersectionPoint, end = intEdge.node1.intersectionPoint});
            polyLines.Add(new Line() { start = intEdge.node2.intersectionPoint, end = intEdge.node4.intersectionPoint});
            polyLines.Add(new Line() { start = intEdge.node4.intersectionPoint, end = intEdge.node3.intersectionPoint});
            polyLines.Add(new Line() { start = intEdge.node3.intersectionPoint, end = intEdge.node2.intersectionPoint});
            s4Cells = new DCOctree.DCNode[] {intEdge.node1, intEdge.node2, intEdge.node3, intEdge.node4};
            yield return null;
        }

        yield return new CustomWait(1.0f);
        showIntersectingEdges = false;
        yield return new CustomWait(2.0f);

        // enable final mesh
        var finalGo = new GameObject();
        var mf = finalGo.AddComponent<MeshFilter>();
        var mr = finalGo.AddComponent<MeshRenderer>();
        var mesh = dc.GetMesh();
        mf.sharedMesh = mesh;
        mr.material = new Material(Shader.Find("Standard"));
        mr.material.color = new Color(0.9f, 0.5f, 0.0f, 1.0f);

        yield return new CustomWait(2.0f);
        drawPolyLines = false;
        showVerts = false;

        yield return new CustomWait(5.0f);


        label = QuickLabels.ShowLabelWithBackground(new Rect(100.0f * textSizeMult, 400.0f * textSizeMult, 1000.0f * textSizeMult, 200.0f * textSizeMult), "The paper also talks about how to simplify meshes, how to traverse the edges in an octree etc. \nSources linked in description.", textSizeMult * 32, Color.white, bgColor);

        yield return new CustomWait(10.0f);

        sw.Stop();
        Debug.LogFormat("Animation took {0} seconds", sw.Elapsed.TotalSeconds);

        Debug.Break();

        yield return null;
    }

    bool showVerts = false;
    int showVertsCount = 0;

    bool showCellEdges = true;

    bool showIntersectingEdges = false;

    bool drawPolyLines = false;

    int x = 5;
    int y = 8;
    int z = 1;

    int curtest = 0;

    bool show4Cells = false;
    DCOctree.DCNode[] s4Cells;

    // good 3 2

    void Update()
    {
        CustomCoroutines.UpdateRoutines();

        if(showGrid)
            QuickShapes.DrawGrid3D(new Vector3(-8.0f, -8.0f, -8.0f), 1.0f, 16, 0.005f, new Color(0.4f, 0.4f, 0.4f, 1.0f));

        //DrawCubeIntersections(new Vector3(x - 8.0f, 0.0f, z - 8.0f), 1.0f);
        if(showCellEdges)
            QuickShapes.DrawCubeEdges(new Vector3(x - 8.0f + 0.5f, y - 8.0f + 0.5f, z - 8.0f + 0.5f), 1.0f, 0.02f, Color.blue);

        if(Input.GetKeyDown(KeyCode.F1))
        {
            x++;
            Debug.LogFormat("X {0} Z {1}", x, z);
        }
        if(Input.GetKeyDown(KeyCode.F2))
        {
            z++;
            Debug.LogFormat("X {0} Z {1}", x, z);
        }

        foreach(var pnorm in pnormals)
        {
            QuickShapes.DrawSphere(pnorm.pos, 0.08f, new Color(0.45f, 0.1f, 0.5f, 1.0f));
            QuickShapes.Draw3DArrow(pnorm.pos, pnorm.pos+pnorm.normal*0.25f, 0.01f, Color.green);
        }

        if(drawLine1)
        {
            QuickShapes.DrawLine(line1.start, line1.end, 0.021f, line1.color);
        }

        if(showQefR1)
        {
            Color tcol = new Color(0.9f, 0.1f, 0.0f, 1.0f);
            QuickShapes.DrawSphere(qefR1, 0.1f, tcol); 
            //QuickShapes.DrawSphere(qefR1MP, 0.1f, Color.black); 
        }

        if(showVerts)
        {
            for(int i = 0; i < showVertsCount; i++)
            {
                Color tcol = new Color(0.9f, 0.1f, 0.0f, 1.0f);
                QuickShapes.DrawSphere(dc._animVertices[i], 0.1f, tcol); 
            }
        }

        if(showIntersectingEdges)
        {
            int count = dc._intEdges.Count;
            for(int i = 0; i < count; i++)
            {
                QuickShapes.DrawLine(dc._intEdges[i].start, dc._intEdges[i].end, 0.03f, Color.green);
                QuickShapes.DrawLine(dc._intEdges[curtest].start, dc._intEdges[curtest].end, 0.041f, Color.red);
            }
        }

        if(show4Cells)
        {
            foreach(var curnode in s4Cells)
            {
                QuickShapes.DrawCubeEdges(curnode.minCorner + new Vector3(0.5f, 0.5f, 0.5f), 1.0f, 0.032f, Color.blue);
            }
        }

        if(drawPolyLines)
        {
            for(int i = 0; i < polyLines.Count; i++)
            {
                QuickShapes.DrawLine(polyLines[i].start, polyLines[i].end, 0.04f, new Color(0.0f, 0.0f, 0.0f, 1.0f));
            }
        }

        if(Input.GetKeyDown(KeyCode.F9))
        {
            if(Cursor.lockState == CursorLockMode.None)
                Cursor.lockState = CursorLockMode.Locked;
            else
                Cursor.lockState = CursorLockMode.None;
        }

        if(Input.GetKeyDown(KeyCode.Return))
        {
            curtest++;
            Debug.Log(curtest);
        }
    }
}
