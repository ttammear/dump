using System;
using System.Collections;
using System.Collections.Generic;
using System.Text;
using UnityEngine;

public class MarchingCubesAnimation : MonoBehaviour
{

    CustomTransform cubeTransform = new CustomTransform();

    public bool offlineRender = false;

    public Vector3 startPosition;
    public Vector3 startRotation;

    public Vector3 bigStart;
    public Vector3 bigStartRot;

    public Vector3 originalPosition;
    public Quaternion originalRotation;

    void Start()
    {
        cubeTransform.position = new Vector3(-1.0f, -1.0f, -1.0f);
    }

    float animationSpeed = 1.0f;

    int stage = 0;

    const int slowCount = 4;
    int cornerCounter = 0;

    int[] booleans = new int[8] { 1, 0, 1, 0, 1, 1, 0, 0 };
    List<Vector3> exampleVerts = new List<Vector3>();
    bool blueBall = false;
    Vector3 blueBallPos;

    List<Vector3> lastVerts = new List<Vector3>();
    List<Vector3> lastVertsInterpolated = new List<Vector3>();
    List<Vector3> lastVertsAnimated = new List<Vector3>();

    float dist = 1.5f;



    IEnumerator PlayAnimation()
    {
        originalPosition = Camera.main.transform.position;
        originalRotation = Camera.main.transform.rotation;

        System.Diagnostics.Stopwatch sw = new System.Diagnostics.Stopwatch();
        sw.Start();

        int label;
        int counter = 0;
        label = QuickLabels.ShowLabel(new Vector2(50.0f, 50.0f), "Marching cubes", 50, Color.black);
        yield return StartCoroutine(OrbitOperation.OrbitTarget(Camera.main.transform, Vector3.zero, 6.0f, 360.0f, 5.0f));


        for (int i = 0; i < 3; i++)
        {
            for (int j = 0; j < 3; j++)
            {
                for (int k = 0; k < 3; k++)
                {
                    yield return StartCoroutine(
                      MoveOperation.MoveTransform(
                        cubeTransform,
                        cubeTransform.position,
                        new Vector3(-1.0f + i, -1.0f + j, -1.0f + k),
                        animationSpeed * 1.0f)
                      );
                    yield return new CustomWait(0.15 * (double)animationSpeed);
                    MarchCube(new Vector3(-1.5f + i, -1.5f + j, -1.5f + k));
                    _vertices.AddRange(lastVerts);
                    yield return new CustomWait(0.15 * (double)animationSpeed);
                    counter++;
                    if (counter >= slowCount)
                        animationSpeed = 0.2f;
                }
            }
        }
        animationSpeed = 1.0f;
        stage = 1;
        yield return StartCoroutine(OrbitOperation.OrbitTarget(Camera.main.transform, Vector3.zero, 6.0f, 180.0f, 1.5f));
        stage = 2;
        yield return StartCoroutine(OrbitOperation.OrbitTarget(Camera.main.transform, Vector3.zero, 6.0f, 180.0f, 1.5f));
        QuickLabels.HideLabel(label);
        stage = 3;
        yield return new CustomWait(0.01);
        label = QuickLabels.ShowLabel(new Vector2(50.0f, 50.0f), "Implementation", 50, Color.black);

        string text =
        @"8 booleans are calculated where each boolean represents a cube corner either being " +
        "inside or outside the volume. These booleans are then combined to a 8 bit bitfield. " +
        "The bitfield forms a case index with 2⁸=256 possible values.";
        int textLabel = QuickLabels.ShowLabel(new Vector2(50.0f, 150.0f), text, 40, Color.black, 1200.0f);


        yield return new CustomWait(8.0);

        stage = 4;
        Camera.main.transform.position = startPosition;
        Camera.main.transform.eulerAngles = startRotation;
        string booleanString = "";
        int blabel = -1;
        yield return new CustomWait(2.0);


        for (int j = 0; j < 1; j++)
        {
            for (int i = 0; i < 8; i++)
            {
                cornerCounter = i;
                booleanString = booleans[i] + booleanString;
                blabel = QuickLabels.ShowLabel(new Vector2(50.0f, 460.0f), booleanString, 40, Color.black, 200.0f, TextAnchor.UpperRight);
                yield return StartCoroutine(new CustomWait(0.9));
                if (blabel != -1)
                    QuickLabels.HideLabel(blabel);
            }

            booleanString += " = 53";
            blabel = QuickLabels.ShowLabel(new Vector2(50.0f, 450.0f), booleanString, 40, Color.black, 300.0f, TextAnchor.UpperRight);
            yield return StartCoroutine(new CustomWait(5.0));

            booleanString = "";
            QuickLabels.HideLabel(blabel);
        }

        stage = 5;

        int[] labels = new int[12];

        QuickLabels.HideLabel(textLabel);
        text =
        "All generated vertices lie on the 12 edges of the cube. We use a lookup table where each row of the table " +
        "contains a list of edge indices for the vertices. Since we have 256 cases we need 256 rows. " +
        "-1 indicates the end of the sequence.";
        textLabel = QuickLabels.ShowLabel(new Vector2(50.0f, 150.0f), text, 40, Color.black, 1200.0f);

        text = "triangleTable[53] = {10, 1, 2, 9, 5, 0, 5, 3, 0, 5, 7, 3, -1, -1, -1, -1}";
        blabel = QuickLabels.ShowLabel(new Vector2(50.0f, 450.0f), text, 40, Color.black, 1200.0f);

        for (int i = 0; i < 12; i++)
        {
            Vector3 offset = (edgeVertexOffsets[i, 0] + edgeVertexOffsets[i, 1]) / 2.0f;
            labels[i] = QuickLabels.Show3DLabel(new Vector3(-0.5f, -0.5f, -0.5f) + offset + Vector3.up * 0.1f + Vector3.right * 0.1f, (i).ToString(), 30, Color.red);
        }
        yield return new CustomWait(10.0);

        for (int i = 0; triangleTable[53, i] != -1; i++)
        {
            StringBuilder sb = new StringBuilder();
            sb.Append("triangleTable[53] = {");
            QuickLabels.HideLabel(blabel);
            for (int j = 0; j < 16; j++)
            {
                if (j != i)
                    sb.Append(triangleTable[53, j] + ", ");
                else
                    sb.Append("<b>" + triangleTable[53, j] + "</b>" + ", ");
            }
            sb.Remove(sb.Length - 2, 2);
            sb.Append("}");
            blabel = QuickLabels.ShowLabel(new Vector3(50.0f, 450.0f), sb.ToString(), 40, Color.black, 1200.0f);

            int vert = triangleTable[53, i];
            Vector3 offset = (edgeVertexOffsets[vert, 0] + edgeVertexOffsets[vert, 1]) / 2.0f;
            blueBallPos = new Vector3(-0.5f, -0.5f, -0.5f) + offset;
            exampleVerts.Add(blueBallPos);
            blueBall = true;
            yield return new CustomWait(2.0);
        }

        yield return new CustomWait(2.0);
        foreach (var llbl in labels)
            QuickLabels.Hide3DLabel(llbl);
        QuickLabels.HideLabel(blabel);
        QuickLabels.HideLabel(textLabel);
        text = "The vertices don't have to be in the middle of the edge. If we have a continuous " +
          "field of values we can interpolate the vertex position along the edge based on the difference " +
          "between the sample values of the cube corners. In case the difference is 0, the vertex is exactly in the middle of the edge." +
          "This gives us more accurate final mesh";
        textLabel = QuickLabels.ShowLabel(new Vector2(50.0f, 150.0f), text, 40, Color.black, 1200.0f);
        stage = 6;
        text = "if((d1 - d2) != 0.0)\n    t = d1 / (d1 - d2)\nelse\n    t = 0.5";
        blabel = QuickLabels.ShowLabel(new Vector2(50.0f, 450.0f), text, 40, Color.black, 1200.0f);

        yield return new CustomWait(15.0);
        QuickLabels.HideLabel(textLabel);
        QuickLabels.HideLabel(blabel);

        Camera.main.transform.position = originalPosition;
        Camera.main.transform.rotation = originalRotation;
        Camera.main.transform.LookAt(Vector3.zero);

        cubeTransform.position = new Vector3(-1.0f, -1.0f, -1.0f);

        stage = 7;
        _vertices.Clear();
        counter = 0;
        for (int i = 0; i < 3; i++)
        {
            for (int j = 0; j < 3; j++)
            {
                for (int k = 0; k < 3; k++)
                {
                    yield return StartCoroutine(
                    MoveOperation.MoveTransform(
                    cubeTransform,
                    cubeTransform.position,
                    new Vector3(-1.0f + i, -1.0f + j, -1.0f + k),
                    animationSpeed * 1.0f)
                    );
                    yield return new CustomWait(0.15 * (double)animationSpeed);
                    MarchCube(new Vector3(-1.5f + i, -1.5f + j, -1.5f + k));
                    float t = 0;
                    while (t < 1.0f)
                    {
                        lastVertsAnimated.Clear();
                        for (int l = 0; l < lastVerts.Count; l++)
                        {
                            lastVertsAnimated.Add(Vector3.Lerp(lastVerts[l], lastVertsInterpolated[l], Mathf.Min(t, 1.0f)));
                        }
                        t += (float)MyTime.dt * (1.0f / animationSpeed);
                        yield return null;
                    }
                    _vertices.AddRange(lastVertsInterpolated);
                    yield return new CustomWait(0.15 * (double)animationSpeed);
                    counter++;
                    if (counter >= 4)
                        animationSpeed = 0.2f;
                }
            }
        }
        stage = 8;
        yield return StartCoroutine(OrbitOperation.OrbitTarget(Camera.main.transform, Vector3.zero, 6.0f, 180.0f, 1.5f));
        stage = 9;
        yield return StartCoroutine(OrbitOperation.OrbitTarget(Camera.main.transform, Vector3.zero, 6.0f, 180.0f, 1.5f));


        animationSpeed = 0.0f;
        QuickLabels.HideLabel(label);
        cubeTransform.position = new Vector3(-8.5f, -8.5f, -8.5f);

        Camera.main.transform.position = bigStart;
        Camera.main.transform.eulerAngles = bigStartRot;

        _vertices.Clear();
        dist = 8.0f;
        stage = 10;
        for (int i = 0; i < 16; i++)
        {
            for (int j = 0; j < 16; j++)
            {
                for (int k = 0; k < 16; k++)
                {
                    cubeTransform.position = new Vector3(-dist + 0.5f + i, -dist + 0.5f + j, -dist + 0.5f + k);
                    MarchCube(new Vector3(-dist + i, -dist + j, -dist + k));
                    _vertices.AddRange(lastVertsInterpolated);
                    yield return null;
                    counter++;
                    if (counter >= 4)
                        animationSpeed = animationSpeed * 0.2f;
                }
            }
        }
        stage = 11;
        yield return StartCoroutine(OrbitOperation.OrbitTarget(Camera.main.transform, Vector3.zero, 6.0f, 180.0f, 1.5f));
        stage = 12;
        yield return StartCoroutine(OrbitOperation.OrbitTarget(Camera.main.transform, Vector3.zero, 6.0f, 180.0f, 1.5f));


        animationSpeed = animationSpeed * 5.0f;


        //var orbit = OrbitOperation.OrbitTarget(Camera.main.transform, Vector3.zero, Vector3.Distance(Vector3.zero, Camera.main.transform.position), 360, 5.0f, true);
        sw.Stop();
        Debug.LogFormat("Animation took {0} seconds", sw.Elapsed.TotalSeconds);

        Debug.Break();

        yield return null;
    }

    float SampleValue(Vector3 pos)
    {
        return Vector3.Distance(Vector3.zero, pos) - dist;
    }

    List<Vector3> _vertices = new List<Vector3>();

    void MarchCube(Vector3 pos)
    {
        lastVerts.Clear();
        lastVertsInterpolated.Clear();

        int caseIndex = 0;
        for (int i = 0; i < 8; i++)
        {
            float sample = SampleValue(pos + cornerOffsets[i]);
            if (sample >= 0.0f)
                caseIndex |= 1 << i;
        }
        if (caseIndex == 0 || caseIndex == 0xFF)
            return;
        int caseVert = 0;

        for (int i = 0; i < 5; i++)
        {
            for (int tri = 0; tri < 3; tri++)
            {
                int edgeCase = triangleTable[caseIndex, caseVert];
                if (edgeCase == -1)
                    return;
                Vector3 vert1 = pos + edgeVertexOffsets[edgeCase, 0];
                Vector3 vert2 = pos + edgeVertexOffsets[edgeCase, 1];
                Vector3 vertPos = (vert1 + vert2) / 2.0f;
                float s1 = SampleValue(pos + edgeVertexOffsets[edgeCase, 0]);
                float s2 = SampleValue(pos + edgeVertexOffsets[edgeCase, 1]);
                float dif = s1 - s2;
                if (dif == 0.0f)
                    dif = 0.5f;
                else
                    dif = s1 / dif;
                Vector3 vertPosInterpolated = vert1 + ((vert2 - vert1) * dif);

                lastVerts.Add(vertPos);
                lastVertsInterpolated.Add(vertPosInterpolated);
                //_vertices.Add(vertPos);
                caseVert++;
            }
        }
    }

    // Update is called once per frame
    void Update()
    {

        //QuickShapes.DrawVertices(mref.vertices, Color.yellow);
        //Debug.Log("Draw " + mref.vertices.Count);

        MoveOperation.UpdateOperations((float)MyTime.dt);
        OrbitOperation.UpdateOperations((float)MyTime.dt);

        if (Input.GetKeyDown(KeyCode.Return))
            StartCoroutine(PlayAnimation());

        Color col = new Color(0.0f, 1.0f, 0.0f, 0.5f);

        if (stage == 0 || stage == 7)
        {
            QuickShapes.DrawCube(cubeTransform.position, 0.95f, col);
            QuickShapes.DrawCubeEdges(cubeTransform.position, 1.0f, 0.04f, Color.black);
            QuickShapes.DrawGrid3D(new Vector3(-1.5f, -1.5f, -1.5f), 1.0f, 3, 0.02f, Color.red);
        }
        if (_vertices.Count >= 3 && (stage < 2 || stage == 7 || stage == 8 || stage == 10 || stage == 11))
            QuickShapes.DrawTriangleEdges(_vertices, 0.03f, Color.blue);
        if (_vertices.Count >= 3 && (stage == 2 || stage == 9 || stage == 12))
            QuickShapes.DrawVertices(_vertices, Color.blue);
        if (stage == 4)
        {
            QuickShapes.DrawCubeEdges(Vector3.zero, 1.0f, 0.02f, Color.black);
            QuickShapes.DrawSphere(cornerOffsets[cornerCounter] - new Vector3(0.5f, 0.5f, 0.5f), 0.2f, Color.red);
            for (int i = 0; i < 8; i++)
            {
                if (booleans[i] == 1)
                    QuickShapes.DrawSphere(cornerOffsets[i] - new Vector3(0.5f, 0.5f, 0.5f), 0.1f, Color.green);
            }
            //QuickShapes.DrawSphere(Vector3.zero, 1.0f, Color.red);
        }

        if (stage == 5)
        {
            QuickShapes.DrawCubeEdges(Vector3.zero, 1.0f, 0.02f, Color.black);
            for (int i = 0; i < 12; i++)
            {
                Vector3 offset = (edgeVertexOffsets[i, 0] + edgeVertexOffsets[i, 1]) / 2.0f;
                QuickShapes.DrawSphere(new Vector3(-0.5f, -0.5f, -0.5f) + offset, 0.1f, Color.red);
            }
            QuickShapes.DrawTriangleEdges(exampleVerts, 0.02f, Color.blue);
            if (blueBall)
                QuickShapes.DrawSphere(blueBallPos, 0.2f, Color.blue);


        }

        if (stage == 7)
        {
            QuickShapes.DrawTriangleEdges(lastVertsAnimated, 0.03f, Color.blue);
        }
        if (stage == 10)
        {
            QuickShapes.DrawCube(cubeTransform.position, 0.95f, col);
            QuickShapes.DrawCubeEdges(cubeTransform.position, 1.0f, 0.04f, Color.black);
            //QuickShapes.DrawGrid3D(new Vector3(-dist, -dist, -dist), 1.0f, Mathf.RoundToInt(dist)*2, 0.02f, Color.red);

        }
    }

    Vector3[] cornerOffsets = new Vector3[8]
    {
    new Vector3(0.0f, 0.0f, 0.0f),
    new Vector3(1.0f, 0.0f, 0.0f),
    new Vector3(1.0f, 1.0f, 0.0f),
    new Vector3(0.0f, 1.0f, 0.0f),
    new Vector3(0.0f, 0.0f, 1.0f),
    new Vector3(1.0f, 0.0f, 1.0f),
    new Vector3(1.0f, 1.0f, 1.0f),
    new Vector3(0.0f, 1.0f, 1.0f)
    };

    Vector3[,] edgeVertexOffsets = new Vector3[12, 2]
    {
    { new Vector3(0.0f, 0.0f, 0.0f), new Vector3(1.0f, 0.0f, 0.0f) },
    { new Vector3(1.0f, 0.0f, 0.0f), new Vector3(1.0f, 1.0f, 0.0f) },
    { new Vector3(0.0f, 1.0f, 0.0f), new Vector3(1.0f, 1.0f, 0.0f) },
    { new Vector3(0.0f, 0.0f, 0.0f), new Vector3(0.0f, 1.0f, 0.0f) },
    { new Vector3(0.0f, 0.0f, 1.0f), new Vector3(1.0f, 0.0f, 1.0f) },
    { new Vector3(1.0f, 0.0f, 1.0f), new Vector3(1.0f, 1.0f, 1.0f) },
    { new Vector3(0.0f, 1.0f, 1.0f), new Vector3(1.0f, 1.0f, 1.0f) },
    { new Vector3(0.0f, 0.0f, 1.0f), new Vector3(0.0f, 1.0f, 1.0f) },
    { new Vector3(0.0f, 0.0f, 0.0f), new Vector3(0.0f, 0.0f, 1.0f) },
    { new Vector3(1.0f, 0.0f, 0.0f), new Vector3(1.0f, 0.0f, 1.0f) },
    { new Vector3(1.0f, 1.0f, 0.0f), new Vector3(1.0f, 1.0f, 1.0f) },
    { new Vector3(0.0f, 1.0f, 0.0f), new Vector3(0.0f, 1.0f, 1.0f) }
    };

    int[,] triangleTable = new int[,]
    {
    {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {0, 8, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {0, 1, 9, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {1, 8, 3, 9, 8, 1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {1, 2, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {0, 8, 3, 1, 2, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {9, 2, 10, 0, 2, 9, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {2, 8, 3, 2, 10, 8, 10, 9, 8, -1, -1, -1, -1, -1, -1, -1},
    {3, 11, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {0, 11, 2, 8, 11, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {1, 9, 0, 2, 3, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {1, 11, 2, 1, 9, 11, 9, 8, 11, -1, -1, -1, -1, -1, -1, -1},
    {3, 10, 1, 11, 10, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {0, 10, 1, 0, 8, 10, 8, 11, 10, -1, -1, -1, -1, -1, -1, -1},
    {3, 9, 0, 3, 11, 9, 11, 10, 9, -1, -1, -1, -1, -1, -1, -1},
    {9, 8, 10, 10, 8, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {4, 7, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {4, 3, 0, 7, 3, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {0, 1, 9, 8, 4, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {4, 1, 9, 4, 7, 1, 7, 3, 1, -1, -1, -1, -1, -1, -1, -1},
    {1, 2, 10, 8, 4, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {3, 4, 7, 3, 0, 4, 1, 2, 10, -1, -1, -1, -1, -1, -1, -1},
    {9, 2, 10, 9, 0, 2, 8, 4, 7, -1, -1, -1, -1, -1, -1, -1},
    {2, 10, 9, 2, 9, 7, 2, 7, 3, 7, 9, 4, -1, -1, -1, -1},
    {8, 4, 7, 3, 11, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {11, 4, 7, 11, 2, 4, 2, 0, 4, -1, -1, -1, -1, -1, -1, -1},
    {9, 0, 1, 8, 4, 7, 2, 3, 11, -1, -1, -1, -1, -1, -1, -1},
    {4, 7, 11, 9, 4, 11, 9, 11, 2, 9, 2, 1, -1, -1, -1, -1},
    {3, 10, 1, 3, 11, 10, 7, 8, 4, -1, -1, -1, -1, -1, -1, -1},
    {1, 11, 10, 1, 4, 11, 1, 0, 4, 7, 11, 4, -1, -1, -1, -1},
    {4, 7, 8, 9, 0, 11, 9, 11, 10, 11, 0, 3, -1, -1, -1, -1},
    {4, 7, 11, 4, 11, 9, 9, 11, 10, -1, -1, -1, -1, -1, -1, -1},
    {9, 5, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {9, 5, 4, 0, 8, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {0, 5, 4, 1, 5, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {8, 5, 4, 8, 3, 5, 3, 1, 5, -1, -1, -1, -1, -1, -1, -1},
    {1, 2, 10, 9, 5, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {3, 0, 8, 1, 2, 10, 4, 9, 5, -1, -1, -1, -1, -1, -1, -1},
    {5, 2, 10, 5, 4, 2, 4, 0, 2, -1, -1, -1, -1, -1, -1, -1},
    {2, 10, 5, 3, 2, 5, 3, 5, 4, 3, 4, 8, -1, -1, -1, -1},
    {9, 5, 4, 2, 3, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {0, 11, 2, 0, 8, 11, 4, 9, 5, -1, -1, -1, -1, -1, -1, -1},
    {0, 5, 4, 0, 1, 5, 2, 3, 11, -1, -1, -1, -1, -1, -1, -1},
    {2, 1, 5, 2, 5, 8, 2, 8, 11, 4, 8, 5, -1, -1, -1, -1},
    {10, 3, 11, 10, 1, 3, 9, 5, 4, -1, -1, -1, -1, -1, -1, -1},
    {4, 9, 5, 0, 8, 1, 8, 10, 1, 8, 11, 10, -1, -1, -1, -1},
    {5, 4, 0, 5, 0, 11, 5, 11, 10, 11, 0, 3, -1, -1, -1, -1},
    {5, 4, 8, 5, 8, 10, 10, 8, 11, -1, -1, -1, -1, -1, -1, -1},
    {9, 7, 8, 5, 7, 9, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {9, 3, 0, 9, 5, 3, 5, 7, 3, -1, -1, -1, -1, -1, -1, -1},
    {0, 7, 8, 0, 1, 7, 1, 5, 7, -1, -1, -1, -1, -1, -1, -1},
    {1, 5, 3, 3, 5, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {9, 7, 8, 9, 5, 7, 10, 1, 2, -1, -1, -1, -1, -1, -1, -1},
    {10, 1, 2, 9, 5, 0, 5, 3, 0, 5, 7, 3, -1, -1, -1, -1},
    {8, 0, 2, 8, 2, 5, 8, 5, 7, 10, 5, 2, -1, -1, -1, -1},
    {2, 10, 5, 2, 5, 3, 3, 5, 7, -1, -1, -1, -1, -1, -1, -1},
    {7, 9, 5, 7, 8, 9, 3, 11, 2, -1, -1, -1, -1, -1, -1, -1},
    {9, 5, 7, 9, 7, 2, 9, 2, 0, 2, 7, 11, -1, -1, -1, -1},
    {2, 3, 11, 0, 1, 8, 1, 7, 8, 1, 5, 7, -1, -1, -1, -1},
    {11, 2, 1, 11, 1, 7, 7, 1, 5, -1, -1, -1, -1, -1, -1, -1},
    {9, 5, 8, 8, 5, 7, 10, 1, 3, 10, 3, 11, -1, -1, -1, -1},
    {5, 7, 0, 5, 0, 9, 7, 11, 0, 1, 0, 10, 11, 10, 0, -1},
    {11, 10, 0, 11, 0, 3, 10, 5, 0, 8, 0, 7, 5, 7, 0, -1},
    {11, 10, 5, 7, 11, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {10, 6, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {0, 8, 3, 5, 10, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {9, 0, 1, 5, 10, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {1, 8, 3, 1, 9, 8, 5, 10, 6, -1, -1, -1, -1, -1, -1, -1},
    {1, 6, 5, 2, 6, 1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {1, 6, 5, 1, 2, 6, 3, 0, 8, -1, -1, -1, -1, -1, -1, -1},
    {9, 6, 5, 9, 0, 6, 0, 2, 6, -1, -1, -1, -1, -1, -1, -1},
    {5, 9, 8, 5, 8, 2, 5, 2, 6, 3, 2, 8, -1, -1, -1, -1},
    {2, 3, 11, 10, 6, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {11, 0, 8, 11, 2, 0, 10, 6, 5, -1, -1, -1, -1, -1, -1, -1},
    {0, 1, 9, 2, 3, 11, 5, 10, 6, -1, -1, -1, -1, -1, -1, -1},
    {5, 10, 6, 1, 9, 2, 9, 11, 2, 9, 8, 11, -1, -1, -1, -1},
    {6, 3, 11, 6, 5, 3, 5, 1, 3, -1, -1, -1, -1, -1, -1, -1},
    {0, 8, 11, 0, 11, 5, 0, 5, 1, 5, 11, 6, -1, -1, -1, -1},
    {3, 11, 6, 0, 3, 6, 0, 6, 5, 0, 5, 9, -1, -1, -1, -1},
    {6, 5, 9, 6, 9, 11, 11, 9, 8, -1, -1, -1, -1, -1, -1, -1},
    {5, 10, 6, 4, 7, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {4, 3, 0, 4, 7, 3, 6, 5, 10, -1, -1, -1, -1, -1, -1, -1},
    {1, 9, 0, 5, 10, 6, 8, 4, 7, -1, -1, -1, -1, -1, -1, -1},
    {10, 6, 5, 1, 9, 7, 1, 7, 3, 7, 9, 4, -1, -1, -1, -1},
    {6, 1, 2, 6, 5, 1, 4, 7, 8, -1, -1, -1, -1, -1, -1, -1},
    {1, 2, 5, 5, 2, 6, 3, 0, 4, 3, 4, 7, -1, -1, -1, -1},
    {8, 4, 7, 9, 0, 5, 0, 6, 5, 0, 2, 6, -1, -1, -1, -1},
    {7, 3, 9, 7, 9, 4, 3, 2, 9, 5, 9, 6, 2, 6, 9, -1},
    {3, 11, 2, 7, 8, 4, 10, 6, 5, -1, -1, -1, -1, -1, -1, -1},
    {5, 10, 6, 4, 7, 2, 4, 2, 0, 2, 7, 11, -1, -1, -1, -1},
    {0, 1, 9, 4, 7, 8, 2, 3, 11, 5, 10, 6, -1, -1, -1, -1},
    {9, 2, 1, 9, 11, 2, 9, 4, 11, 7, 11, 4, 5, 10, 6, -1},
    {8, 4, 7, 3, 11, 5, 3, 5, 1, 5, 11, 6, -1, -1, -1, -1},
    {5, 1, 11, 5, 11, 6, 1, 0, 11, 7, 11, 4, 0, 4, 11, -1},
    {0, 5, 9, 0, 6, 5, 0, 3, 6, 11, 6, 3, 8, 4, 7, -1},
    {6, 5, 9, 6, 9, 11, 4, 7, 9, 7, 11, 9, -1, -1, -1, -1},
    {10, 4, 9, 6, 4, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {4, 10, 6, 4, 9, 10, 0, 8, 3, -1, -1, -1, -1, -1, -1, -1},
    {10, 0, 1, 10, 6, 0, 6, 4, 0, -1, -1, -1, -1, -1, -1, -1},
    {8, 3, 1, 8, 1, 6, 8, 6, 4, 6, 1, 10, -1, -1, -1, -1},
    {1, 4, 9, 1, 2, 4, 2, 6, 4, -1, -1, -1, -1, -1, -1, -1},
    {3, 0, 8, 1, 2, 9, 2, 4, 9, 2, 6, 4, -1, -1, -1, -1},
    {0, 2, 4, 4, 2, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {8, 3, 2, 8, 2, 4, 4, 2, 6, -1, -1, -1, -1, -1, -1, -1},
    {10, 4, 9, 10, 6, 4, 11, 2, 3, -1, -1, -1, -1, -1, -1, -1},
    {0, 8, 2, 2, 8, 11, 4, 9, 10, 4, 10, 6, -1, -1, -1, -1},
    {3, 11, 2, 0, 1, 6, 0, 6, 4, 6, 1, 10, -1, -1, -1, -1},
    {6, 4, 1, 6, 1, 10, 4, 8, 1, 2, 1, 11, 8, 11, 1, -1},
    {9, 6, 4, 9, 3, 6, 9, 1, 3, 11, 6, 3, -1, -1, -1, -1},
    {8, 11, 1, 8, 1, 0, 11, 6, 1, 9, 1, 4, 6, 4, 1, -1},
    {3, 11, 6, 3, 6, 0, 0, 6, 4, -1, -1, -1, -1, -1, -1, -1},
    {6, 4, 8, 11, 6, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {7, 10, 6, 7, 8, 10, 8, 9, 10, -1, -1, -1, -1, -1, -1, -1},
    {0, 7, 3, 0, 10, 7, 0, 9, 10, 6, 7, 10, -1, -1, -1, -1},
    {10, 6, 7, 1, 10, 7, 1, 7, 8, 1, 8, 0, -1, -1, -1, -1},
    {10, 6, 7, 10, 7, 1, 1, 7, 3, -1, -1, -1, -1, -1, -1, -1},
    {1, 2, 6, 1, 6, 8, 1, 8, 9, 8, 6, 7, -1, -1, -1, -1},
    {2, 6, 9, 2, 9, 1, 6, 7, 9, 0, 9, 3, 7, 3, 9, -1},
    {7, 8, 0, 7, 0, 6, 6, 0, 2, -1, -1, -1, -1, -1, -1, -1},
    {7, 3, 2, 6, 7, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {2, 3, 11, 10, 6, 8, 10, 8, 9, 8, 6, 7, -1, -1, -1, -1},
    {2, 0, 7, 2, 7, 11, 0, 9, 7, 6, 7, 10, 9, 10, 7, -1},
    {1, 8, 0, 1, 7, 8, 1, 10, 7, 6, 7, 10, 2, 3, 11, -1},
    {11, 2, 1, 11, 1, 7, 10, 6, 1, 6, 7, 1, -1, -1, -1, -1},
    {8, 9, 6, 8, 6, 7, 9, 1, 6, 11, 6, 3, 1, 3, 6, -1},
    {0, 9, 1, 11, 6, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {7, 8, 0, 7, 0, 6, 3, 11, 0, 11, 6, 0, -1, -1, -1, -1},
    {7, 11, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {7, 6, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {3, 0, 8, 11, 7, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {0, 1, 9, 11, 7, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {8, 1, 9, 8, 3, 1, 11, 7, 6, -1, -1, -1, -1, -1, -1, -1},
    {10, 1, 2, 6, 11, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {1, 2, 10, 3, 0, 8, 6, 11, 7, -1, -1, -1, -1, -1, -1, -1},
    {2, 9, 0, 2, 10, 9, 6, 11, 7, -1, -1, -1, -1, -1, -1, -1},
    {6, 11, 7, 2, 10, 3, 10, 8, 3, 10, 9, 8, -1, -1, -1, -1},
    {7, 2, 3, 6, 2, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {7, 0, 8, 7, 6, 0, 6, 2, 0, -1, -1, -1, -1, -1, -1, -1},
    {2, 7, 6, 2, 3, 7, 0, 1, 9, -1, -1, -1, -1, -1, -1, -1},
    {1, 6, 2, 1, 8, 6, 1, 9, 8, 8, 7, 6, -1, -1, -1, -1},
    {10, 7, 6, 10, 1, 7, 1, 3, 7, -1, -1, -1, -1, -1, -1, -1},
    {10, 7, 6, 1, 7, 10, 1, 8, 7, 1, 0, 8, -1, -1, -1, -1},
    {0, 3, 7, 0, 7, 10, 0, 10, 9, 6, 10, 7, -1, -1, -1, -1},
    {7, 6, 10, 7, 10, 8, 8, 10, 9, -1, -1, -1, -1, -1, -1, -1},
    {6, 8, 4, 11, 8, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {3, 6, 11, 3, 0, 6, 0, 4, 6, -1, -1, -1, -1, -1, -1, -1},
    {8, 6, 11, 8, 4, 6, 9, 0, 1, -1, -1, -1, -1, -1, -1, -1},
    {9, 4, 6, 9, 6, 3, 9, 3, 1, 11, 3, 6, -1, -1, -1, -1},
    {6, 8, 4, 6, 11, 8, 2, 10, 1, -1, -1, -1, -1, -1, -1, -1},
    {1, 2, 10, 3, 0, 11, 0, 6, 11, 0, 4, 6, -1, -1, -1, -1},
    {4, 11, 8, 4, 6, 11, 0, 2, 9, 2, 10, 9, -1, -1, -1, -1},
    {10, 9, 3, 10, 3, 2, 9, 4, 3, 11, 3, 6, 4, 6, 3, -1},
    {8, 2, 3, 8, 4, 2, 4, 6, 2, -1, -1, -1, -1, -1, -1, -1},
    {0, 4, 2, 4, 6, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {1, 9, 0, 2, 3, 4, 2, 4, 6, 4, 3, 8, -1, -1, -1, -1},
    {1, 9, 4, 1, 4, 2, 2, 4, 6, -1, -1, -1, -1, -1, -1, -1},
    {8, 1, 3, 8, 6, 1, 8, 4, 6, 6, 10, 1, -1, -1, -1, -1},
    {10, 1, 0, 10, 0, 6, 6, 0, 4, -1, -1, -1, -1, -1, -1, -1},
    {4, 6, 3, 4, 3, 8, 6, 10, 3, 0, 3, 9, 10, 9, 3, -1},
    {10, 9, 4, 6, 10, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {4, 9, 5, 7, 6, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {0, 8, 3, 4, 9, 5, 11, 7, 6, -1, -1, -1, -1, -1, -1, -1},
    {5, 0, 1, 5, 4, 0, 7, 6, 11, -1, -1, -1, -1, -1, -1, -1},
    {11, 7, 6, 8, 3, 4, 3, 5, 4, 3, 1, 5, -1, -1, -1, -1},
    {9, 5, 4, 10, 1, 2, 7, 6, 11, -1, -1, -1, -1, -1, -1, -1},
    {6, 11, 7, 1, 2, 10, 0, 8, 3, 4, 9, 5, -1, -1, -1, -1},
    {7, 6, 11, 5, 4, 10, 4, 2, 10, 4, 0, 2, -1, -1, -1, -1},
    {3, 4, 8, 3, 5, 4, 3, 2, 5, 10, 5, 2, 11, 7, 6, -1},
    {7, 2, 3, 7, 6, 2, 5, 4, 9, -1, -1, -1, -1, -1, -1, -1},
    {9, 5, 4, 0, 8, 6, 0, 6, 2, 6, 8, 7, -1, -1, -1, -1},
    {3, 6, 2, 3, 7, 6, 1, 5, 0, 5, 4, 0, -1, -1, -1, -1},
    {6, 2, 8, 6, 8, 7, 2, 1, 8, 4, 8, 5, 1, 5, 8, -1},
    {9, 5, 4, 10, 1, 6, 1, 7, 6, 1, 3, 7, -1, -1, -1, -1},
    {1, 6, 10, 1, 7, 6, 1, 0, 7, 8, 7, 0, 9, 5, 4, -1},
    {4, 0, 10, 4, 10, 5, 0, 3, 10, 6, 10, 7, 3, 7, 10, -1},
    {7, 6, 10, 7, 10, 8, 5, 4, 10, 4, 8, 10, -1, -1, -1, -1},
    {6, 9, 5, 6, 11, 9, 11, 8, 9, -1, -1, -1, -1, -1, -1, -1},
    {3, 6, 11, 0, 6, 3, 0, 5, 6, 0, 9, 5, -1, -1, -1, -1},
    {0, 11, 8, 0, 5, 11, 0, 1, 5, 5, 6, 11, -1, -1, -1, -1},
    {6, 11, 3, 6, 3, 5, 5, 3, 1, -1, -1, -1, -1, -1, -1, -1},
    {1, 2, 10, 9, 5, 11, 9, 11, 8, 11, 5, 6, -1, -1, -1, -1},
    {0, 11, 3, 0, 6, 11, 0, 9, 6, 5, 6, 9, 1, 2, 10, -1},
    {11, 8, 5, 11, 5, 6, 8, 0, 5, 10, 5, 2, 0, 2, 5, -1},
    {6, 11, 3, 6, 3, 5, 2, 10, 3, 10, 5, 3, -1, -1, -1, -1},
    {5, 8, 9, 5, 2, 8, 5, 6, 2, 3, 8, 2, -1, -1, -1, -1},
    {9, 5, 6, 9, 6, 0, 0, 6, 2, -1, -1, -1, -1, -1, -1, -1},
    {1, 5, 8, 1, 8, 0, 5, 6, 8, 3, 8, 2, 6, 2, 8, -1},
    {1, 5, 6, 2, 1, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {1, 3, 6, 1, 6, 10, 3, 8, 6, 5, 6, 9, 8, 9, 6, -1},
    {10, 1, 0, 10, 0, 6, 9, 5, 0, 5, 6, 0, -1, -1, -1, -1},
    {0, 3, 8, 5, 6, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {10, 5, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {11, 5, 10, 7, 5, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {11, 5, 10, 11, 7, 5, 8, 3, 0, -1, -1, -1, -1, -1, -1, -1},
    {5, 11, 7, 5, 10, 11, 1, 9, 0, -1, -1, -1, -1, -1, -1, -1},
    {10, 7, 5, 10, 11, 7, 9, 8, 1, 8, 3, 1, -1, -1, -1, -1},
    {11, 1, 2, 11, 7, 1, 7, 5, 1, -1, -1, -1, -1, -1, -1, -1},
    {0, 8, 3, 1, 2, 7, 1, 7, 5, 7, 2, 11, -1, -1, -1, -1},
    {9, 7, 5, 9, 2, 7, 9, 0, 2, 2, 11, 7, -1, -1, -1, -1},
    {7, 5, 2, 7, 2, 11, 5, 9, 2, 3, 2, 8, 9, 8, 2, -1},
    {2, 5, 10, 2, 3, 5, 3, 7, 5, -1, -1, -1, -1, -1, -1, -1},
    {8, 2, 0, 8, 5, 2, 8, 7, 5, 10, 2, 5, -1, -1, -1, -1},
    {9, 0, 1, 5, 10, 3, 5, 3, 7, 3, 10, 2, -1, -1, -1, -1},
    {9, 8, 2, 9, 2, 1, 8, 7, 2, 10, 2, 5, 7, 5, 2, -1},
    {1, 3, 5, 3, 7, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {0, 8, 7, 0, 7, 1, 1, 7, 5, -1, -1, -1, -1, -1, -1, -1},
    {9, 0, 3, 9, 3, 5, 5, 3, 7, -1, -1, -1, -1, -1, -1, -1},
    {9, 8, 7, 5, 9, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {5, 8, 4, 5, 10, 8, 10, 11, 8, -1, -1, -1, -1, -1, -1, -1},
    {5, 0, 4, 5, 11, 0, 5, 10, 11, 11, 3, 0, -1, -1, -1, -1},
    {0, 1, 9, 8, 4, 10, 8, 10, 11, 10, 4, 5, -1, -1, -1, -1},
    {10, 11, 4, 10, 4, 5, 11, 3, 4, 9, 4, 1, 3, 1, 4, -1},
    {2, 5, 1, 2, 8, 5, 2, 11, 8, 4, 5, 8, -1, -1, -1, -1},
    {0, 4, 11, 0, 11, 3, 4, 5, 11, 2, 11, 1, 5, 1, 11, -1},
    {0, 2, 5, 0, 5, 9, 2, 11, 5, 4, 5, 8, 11, 8, 5, -1},
    {9, 4, 5, 2, 11, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {2, 5, 10, 3, 5, 2, 3, 4, 5, 3, 8, 4, -1, -1, -1, -1},
    {5, 10, 2, 5, 2, 4, 4, 2, 0, -1, -1, -1, -1, -1, -1, -1},
    {3, 10, 2, 3, 5, 10, 3, 8, 5, 4, 5, 8, 0, 1, 9, -1},
    {5, 10, 2, 5, 2, 4, 1, 9, 2, 9, 4, 2, -1, -1, -1, -1},
    {8, 4, 5, 8, 5, 3, 3, 5, 1, -1, -1, -1, -1, -1, -1, -1},
    {0, 4, 5, 1, 0, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {8, 4, 5, 8, 5, 3, 9, 0, 5, 0, 3, 5, -1, -1, -1, -1},
    {9, 4, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {4, 11, 7, 4, 9, 11, 9, 10, 11, -1, -1, -1, -1, -1, -1, -1},
    {0, 8, 3, 4, 9, 7, 9, 11, 7, 9, 10, 11, -1, -1, -1, -1},
    {1, 10, 11, 1, 11, 4, 1, 4, 0, 7, 4, 11, -1, -1, -1, -1},
    {3, 1, 4, 3, 4, 8, 1, 10, 4, 7, 4, 11, 10, 11, 4, -1},
    {4, 11, 7, 9, 11, 4, 9, 2, 11, 9, 1, 2, -1, -1, -1, -1},
    {9, 7, 4, 9, 11, 7, 9, 1, 11, 2, 11, 1, 0, 8, 3, -1},
    {11, 7, 4, 11, 4, 2, 2, 4, 0, -1, -1, -1, -1, -1, -1, -1},
    {11, 7, 4, 11, 4, 2, 8, 3, 4, 3, 2, 4, -1, -1, -1, -1},
    {2, 9, 10, 2, 7, 9, 2, 3, 7, 7, 4, 9, -1, -1, -1, -1},
    {9, 10, 7, 9, 7, 4, 10, 2, 7, 8, 7, 0, 2, 0, 7, -1},
    {3, 7, 10, 3, 10, 2, 7, 4, 10, 1, 10, 0, 4, 0, 10, -1},
    {1, 10, 2, 8, 7, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {4, 9, 1, 4, 1, 7, 7, 1, 3, -1, -1, -1, -1, -1, -1, -1},
    {4, 9, 1, 4, 1, 7, 0, 8, 1, 8, 7, 1, -1, -1, -1, -1},
    {4, 0, 3, 7, 4, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {4, 8, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {9, 10, 8, 10, 11, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {3, 0, 9, 3, 9, 11, 11, 9, 10, -1, -1, -1, -1, -1, -1, -1},
    {0, 1, 10, 0, 10, 8, 8, 10, 11, -1, -1, -1, -1, -1, -1, -1},
    {3, 1, 10, 11, 3, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {1, 2, 11, 1, 11, 9, 9, 11, 8, -1, -1, -1, -1, -1, -1, -1},
    {3, 0, 9, 3, 9, 11, 1, 2, 9, 2, 11, 9, -1, -1, -1, -1},
    {0, 2, 11, 8, 0, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {3, 2, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {2, 3, 8, 2, 8, 10, 10, 8, 9, -1, -1, -1, -1, -1, -1, -1},
    {9, 10, 2, 0, 9, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {2, 3, 8, 2, 8, 10, 0, 1, 8, 1, 10, 8, -1, -1, -1, -1},
    {1, 10, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {1, 3, 8, 9, 1, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {0, 9, 1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {0, 3, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}
    };
}
