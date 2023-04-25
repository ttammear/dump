using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using UnityEngine;

public class DCOctree
{
    public readonly static Vector3[] childOffsets =
    {
        new Vector3(0.0f, 0.0f, 0.0f),
        new Vector3(0.0f, 0.0f, 1.0f),
        new Vector3(0.0f, 1.0f, 0.0f),
        new Vector3(0.0f, 1.0f, 1.0f),
        new Vector3(1.0f, 0.0f, 0.0f),
        new Vector3(1.0f, 0.0f, 1.0f),
        new Vector3(1.0f, 1.0f, 0.0f),
        new Vector3(1.0f, 1.0f, 1.0f)
    };

    public enum DCNodeType
    {
        None,
        Internal,
        Leaf
    }

    public class DCNode
    {
        public Vector3 minCorner;
        public int size;
        public int index;
        public byte cornerBits;
        public Vector3 intersectionPoint;
        public DCNode[] children;
        public DCNodeType type;
    }

    public struct Line
    {
        public Vector3 start;
        public Vector3 end;

        public DCNode node1;
        public DCNode node2;
        public DCNode node3;
        public DCNode node4;
    }

    public readonly DCNode rootNode;
    public readonly int log2Size;

    public List<Vector3> _vertices = new List<Vector3>();
    public List<int> _indices = new List<int>();

    public List<Vector3> _intersections = new List<Vector3>();
    public List<Vector3> _intersectionNormals = new List<Vector3>();

    public List<Vector3> _animVertices = new List<Vector3>();
    public List<Line> _intEdges = new List<Line>();

    public List<Vector3> _finalVertices = new List<Vector3>();
    public List<Vector3> _finalNormals = new List<Vector3>();

    public DCOctree(int log2Size, Vector3 offset)
    {
        int size = 1 << log2Size;
        rootNode = AddNodesRecursive(null, new Vector3(-size / 2.0f, -size / 2.0f, -size / 2.0f) + offset, size);
        this.log2Size = log2Size;
    }

    static float sdTorus(Vector3 p/*, vec2 t */)
    {
        Vector2 t = new Vector2(5.0f, 2.0f);
        float pxzm = new Vector2(p.x, p.z).magnitude;
        Vector2 q = new Vector2(pxzm-t.x, p.y);
        return q.magnitude - t.y;
    }

    static Vector3 sdTorusNormal(Vector3 p)
    {
        float R = 5.0f;
        Vector2 c = Vector2.zero;

        Vector2 v = new Vector2(p.x, p.z);
        float magV = v.magnitude;
        Vector2 a = c + v / magV * R;

        //return (p - a).normalized;
        return (p - new Vector3(a.x, 0.0f, a.y)).normalized;
    }

    static float sphere(Vector3 p)
    {
        return Vector3.Distance(p, Vector3.zero) - 5.0f;
    }

    public static float sampleValue(Vector3 position)
    {
        //return sphere(position);
        return sdTorus(position);
    }

    public DCNode CreateLeaf(DCNode leaf)
    {
        int cornerMask = 0;
        for(int i = 0; i < 8; i++)
        {
            if(sampleValue(leaf.minCorner + childOffsets[i]*leaf.size) < 0.0f)
                cornerMask |= 1 << i;
        }
        // no intersection
        if (cornerMask == 0xFF || cornerMask == 0x00)
            return null;

        QefSolver qef = new QefSolver();
        int crossings = 0;

        for(int i = 0; i < 12 && crossings < 6; i++)
        {
            int ofstI1 = edgevmap[i, 0];
            int ofstI2 = edgevmap[i, 1];
            if(((cornerMask >> ofstI1) & 1) != ((cornerMask >> ofstI2) & 1))
            {
                Vector3 ofst1 = leaf.minCorner + childOffsets[ofstI1];
                Vector3 ofst2 = leaf.minCorner + childOffsets[ofstI2];
                Vector3 ipoint = FindIntersection(ofst1, ofst2);
                Vector3 inormal = sdTorusNormal(ipoint);
                //QuickShapes.DrawSphere(ipoint, 0.1f, Color.red);
                _intersections.Add(ipoint);
                _intersectionNormals.Add(inormal);

                qef.Add(ipoint.x, ipoint.y, ipoint.z, inormal.x, inormal.y, inormal.z);
                crossings++;
            }
        }

        Vector3 qefPos;
        qef.Solve(out qefPos, 1e-6f, 4, 1e-6f);

        Vector3 diPos = new Vector3(qefPos.x, qefPos.y, qefPos.z);
        QefData diQD = qef.GetData();

        Vector3 min = new Vector3(leaf.minCorner.x, leaf.minCorner.y, leaf.minCorner.z);
        Vector3 max = min + new Vector3(leaf.size, leaf.size, leaf.size);
        if (diPos.x < min.x || diPos.x > max.x ||
        diPos.y < min.y || diPos.y > max.y ||
        diPos.z < min.z || diPos.z > max.z)
        {
            //Vector3 mp = qef.massPoint;
            //diPos = new Vector3(mp.x, mp.y, mp.z);

            var dx = Mathf.Max(leaf.minCorner.x - diPos.x, 0, diPos.x - max.x);
            var dy = Mathf.Max(leaf.minCorner.y - diPos.y, 0, diPos.y - max.y);
            var dz = Mathf.Max(leaf.minCorner.z - diPos.z, 0, diPos.z - max.z);
            float distToCell = Mathf.Sqrt(dx*dx + dy*dy + dz*dz);

            Vector3 toMP = (qef.massPoint - diPos).normalized;
            diPos = diPos + (distToCell * toMP);
        }

        leaf.intersectionPoint = diPos;
        _animVertices.Add(diPos);

        leaf.cornerBits = (byte)cornerMask;
        //leaf.intersectionPoint = leaf.minCorner + Vector3.one * leaf.size * 0.5f;
        leaf.type = DCNodeType.Leaf;
        return leaf;
    }

    public static Vector3 FindIntersection(Vector3 start, Vector3 end)
    {
        const int steps = 50;
        Vector3 step = (end - start) / steps;

        for(int i = 0; i < steps; i++)
        {
            Vector3 firstPoint = start + step*i;
            Vector3 secondPoint = start + step*(i+1);
            float fs = sampleValue(firstPoint);
            float ss = sampleValue(secondPoint);
            //Debug.LogFormat("{0} {1} {2}", i, fs, ss);
            if(Mathf.Sign(fs) != Mathf.Sign(ss))
                return (firstPoint + secondPoint) / 2.0f;
        }
        throw new Exception();
    }

    public DCNode AddNodesRecursive(DCNode parent, Vector3 minCorner, int size)
    {
        var node = new DCNode();
        node.minCorner = minCorner;
        node.size = size;
        node.type = DCNodeType.Internal;

        if(size > 1.0f)
        {
            //bool hasChildren = false;
            node.children = new DCNode[8];
            int childCount = 0;
            for(int i = 0; i < 8; i++)
            {
                int csize = size / 2;
                Vector3 offset = childOffsets[i] * csize;
                var child = AddNodesRecursive(node, minCorner + offset, csize);
                node.children[i] = child;
                //hasChildren |= child != null;
                if (child != null)
                    childCount++;
            }
            if (/*hasChildren && */childCount >= 1)
                return node;
            /*else if (childCount == 1)
            {
                node.children = null;
                return CreateLeaf(node);
            }*/
            else
                return null;
        }
        else
        {
            return CreateLeaf(node);
        }
    }

    public void FillVertexBuffer(DCNode node)
    {
        _vertices.Clear();
        if (node.type != DCNodeType.Internal)
        {
            node.index = _vertices.Count;
            _vertices.Add(node.intersectionPoint);
        }
        else if(node.type != DCNodeType.Leaf)
        {
            for (int i = 0; i < 8; i++)
            {
                FillVertexBuffer(node.children[i]);
            }
        }
    }

    public void addEdge(DCNode node1, DCNode node2, DCNode node3, DCNode node4, int direction)
    {
        int edge1 = faceNodeInnerEdge[direction, 0, 0];
        int edge2 = faceNodeInnerEdge[direction, 0, 1];
        Vector3 start = node1.minCorner + childOffsets[edge1];
        Vector3 end = node1.minCorner + childOffsets[edge2];
        _intEdges.Add(new Line() { start = start, end = end , node1 = node1, node2 = node2, node3 = node3, node4 = node4} ); 
    }

    public void processEdge(DCNode node1, DCNode node2, DCNode node3, DCNode node4, int direction)
    {
        debugProcessEdgeCount++;

        addEdge(node1, node2, node3, node4, direction);

        Color col;
        switch(direction)
        {
            case 0:
                col = Color.green;
                break;
            case 1:
                col = Color.green;
                break;
            case 2:
                col = Color.green;
                break;
            default:
                col = Color.white;
                break;
        }

        DCNode minNode = node1;
        minNode = node2.size < minNode.size ? node2 : minNode;
        minNode = node3.size < minNode.size ? node3 : minNode;
        minNode = node4.size < minNode.size ? node4 : minNode;

        // TODO: this is STUPID!
        int minNodeIndex = 0;
        if (minNode == node2)
            minNodeIndex = 1;
        if (minNode == node3)
            minNodeIndex = 2;
        if (minNode == node4)
            minNodeIndex = 3;

        int edge1 = faceNodeInnerEdge[direction, minNodeIndex, 0];
        int edge2 = faceNodeInnerEdge[direction, minNodeIndex, 1];

        // no intersection
        int bits = minNode.cornerBits;
        if (((bits >> edge1) & 1) == ((bits >> edge2) & 1))
            return;

        bool flip = ((bits >> edge1) & 1) != 0;

        Vector3 start;
        Vector3 end;
        switch(direction)
        {
            case 0:
                start = minNode.minCorner+new Vector3(0.0f, minNode.size, minNode.size);
                end = start + new Vector3(minNode.size, 0.0f, 0.0f);
                break;
            case 1:
                start = minNode.minCorner+new Vector3(minNode.size, 0.0f, minNode.size);
                end = start + new Vector3(0.0f, minNode.size, 0.0f);
                break;
            case 2:
                start = minNode.minCorner+new Vector3(minNode.size, minNode.size, 0.0f);
                end = start + new Vector3(0.0f, 0.0f, minNode.size);
                break;
            default:
                start = Vector3.zero;
                end = Vector3.zero;
                break;
        }
        //QuickShapes.DrawLine(start, end, 0.05f, Color.blue);
        if (!flip)
        {
            _finalVertices.Add(node1.intersectionPoint);
            _finalVertices.Add(node2.intersectionPoint);
            _finalVertices.Add(node3.intersectionPoint);
            _finalVertices.Add(node2.intersectionPoint);
            _finalVertices.Add(node4.intersectionPoint);
            _finalVertices.Add(node3.intersectionPoint);
            //QuickShapes.DrawTriangle(node1.intersectionPoint, node2.intersectionPoint, node3.intersectionPoint, col);
            //QuickShapes.DrawTriangle(node2.intersectionPoint, node4.intersectionPoint, node3.intersectionPoint, col);
        }
        else
        {
            _finalVertices.Add(node3.intersectionPoint);
            _finalVertices.Add(node2.intersectionPoint);
            _finalVertices.Add(node1.intersectionPoint);
            _finalVertices.Add(node3.intersectionPoint);
            _finalVertices.Add(node4.intersectionPoint);
            _finalVertices.Add(node2.intersectionPoint);

            //QuickShapes.DrawTriangle(node3.intersectionPoint, node2.intersectionPoint, node1.intersectionPoint, col);
            //QuickShapes.DrawTriangle(node3.intersectionPoint, node4.intersectionPoint, node2.intersectionPoint, col);
        }
    }


    public Mesh GetMesh()
    {
        _finalNormals.Clear();
        int[] indices = new int[_finalVertices.Count];
        for(int i = 0 ; i < _finalVertices.Count; i++)
        {
            _finalNormals.Add(sdTorusNormal(_finalVertices[i]));
            indices[i] = i;
        }

        var ret = new Mesh();
        ret.SetVertices(_finalVertices);
        ret.triangles = indices;
        ret.SetNormals(_finalNormals);
        ret.RecalculateBounds();
        return ret;
    }

    public void edgeProc(DCNode node1, DCNode node2, DCNode node3, DCNode node4, int direction)
    {
        debugEdgeProcCount++;
        if (node1 == null || node2 == null || node3 == null || node4 == null)
            return;

        if (node1.type != DCNodeType.Internal &&
            node2.type != DCNodeType.Internal &&
            node3.type != DCNodeType.Internal &&
            node4.type != DCNodeType.Internal) // all leaves, create face if intersection
        {
            processEdge(node1, node2, node3, node4, direction);
        }
        else // theres at least one internal node, pick inner children until all are leaves
        {
            switch(direction)
            {
                case 0:
                    edgeProc(
                        node1.type != DCNodeType.Internal ? node1 : node1.children[3],
                        node2.type != DCNodeType.Internal ? node2 : node2.children[2],
                        node3.type != DCNodeType.Internal ? node3 : node3.children[1],
                        node4.type != DCNodeType.Internal ? node4 : node4.children[0], 0);
                    edgeProc(
                        node1.type != DCNodeType.Internal ? node1 : node1.children[7],
                        node2.type != DCNodeType.Internal ? node2 : node2.children[6],
                        node3.type != DCNodeType.Internal ? node3 : node3.children[5],
                        node4.type != DCNodeType.Internal ? node4 : node4.children[4], 0);
                    break;
                case 1:
                    edgeProc(
                        node1.type != DCNodeType.Internal ? node1 : node1.children[7],
                        node2.type != DCNodeType.Internal ? node2 : node2.children[3],
                        node3.type != DCNodeType.Internal ? node3 : node3.children[6],
                        node4.type != DCNodeType.Internal ? node4 : node4.children[2], 1);
                    edgeProc(
                        node1.type != DCNodeType.Internal ? node1 : node1.children[5],
                        node2.type != DCNodeType.Internal ? node2 : node2.children[1],
                        node3.type != DCNodeType.Internal ? node3 : node3.children[4],
                        node4.type != DCNodeType.Internal ? node4 : node4.children[0], 1);
                    break;
                case 2:
                    edgeProc(
                        node1.type != DCNodeType.Internal ? node1 : node1.children[6],
                        node2.type != DCNodeType.Internal ? node2 : node2.children[4],
                        node3.type != DCNodeType.Internal ? node3 : node3.children[2],
                        node4.type != DCNodeType.Internal ? node4 : node4.children[0], 2);
                    edgeProc(
                        node1.type != DCNodeType.Internal ? node1 : node1.children[7],
                        node2.type != DCNodeType.Internal ? node2 : node2.children[5],
                        node3.type != DCNodeType.Internal ? node3 : node3.children[3],
                        node4.type != DCNodeType.Internal ? node4 : node4.children[1], 2);
                    break;
                default: // invalid state
                    return;
            }
        }
    }

    public void faceProc(DCNode node1, DCNode node2, int direction)
    {
        debugFaceProcCount++;
        if (node1 == null || node2 == null)
            return;

        if(node1.type == DCNodeType.Internal || node2.type == DCNodeType.Internal)
        {
            switch(direction)
            {
                case 0:
                    faceProc(node1.type == DCNodeType.Internal ? node1.children[4] : node1, node2.type == DCNodeType.Internal ? node2.children[0] : node2, direction);
                    faceProc(node1.type == DCNodeType.Internal ? node1.children[5] : node1, node2.type == DCNodeType.Internal ? node2.children[1] : node2, direction);
                    faceProc(node1.type == DCNodeType.Internal ? node1.children[6] : node1, node2.type == DCNodeType.Internal ? node2.children[2] : node2, direction);
                    faceProc(node1.type == DCNodeType.Internal ? node1.children[7] : node1, node2.type == DCNodeType.Internal ? node2.children[3] : node2, direction);
                    break;
                case 1:
                    faceProc(node1.type == DCNodeType.Internal ? node1.children[2] : node1, node2.type == DCNodeType.Internal ? node2.children[0] : node2, direction);
                    faceProc(node1.type == DCNodeType.Internal ? node1.children[3] : node1, node2.type == DCNodeType.Internal ? node2.children[1] : node2, direction);
                    faceProc(node1.type == DCNodeType.Internal ? node1.children[6] : node1, node2.type == DCNodeType.Internal ? node2.children[4] : node2, direction);
                    faceProc(node1.type == DCNodeType.Internal ? node1.children[7] : node1, node2.type == DCNodeType.Internal ? node2.children[5] : node2, direction);
                    break;
                case 2:
                    faceProc(node1.type == DCNodeType.Internal ? node1.children[1] : node1, node2.type == DCNodeType.Internal ? node2.children[0] : node2, direction);
                    faceProc(node1.type == DCNodeType.Internal ? node1.children[5] : node1, node2.type == DCNodeType.Internal ? node2.children[4] : node2, direction);
                    faceProc(node1.type == DCNodeType.Internal ? node1.children[3] : node1, node2.type == DCNodeType.Internal ? node2.children[2] : node2, direction);
                    faceProc(node1.type == DCNodeType.Internal ? node1.children[7] : node1, node2.type == DCNodeType.Internal ? node2.children[6] : node2, direction);
                    break;
                default: // invalid state
                    return;
            }

            switch(direction)
            {
                case 0:
                    edgeProc(
                    node1.type == DCNodeType.Internal ? node1.children[4] : node1,
                    node1.type == DCNodeType.Internal ? node1.children[6] : node1,
                    node2.type == DCNodeType.Internal ? node2.children[0] : node2,
                    node2.type == DCNodeType.Internal ? node2.children[2] : node2, 2);

                    edgeProc(
                    node1.type == DCNodeType.Internal ? node1.children[5] : node1,
                    node1.type == DCNodeType.Internal ? node1.children[7] : node1,
                    node2.type == DCNodeType.Internal ? node2.children[1] : node2,
                    node2.type == DCNodeType.Internal ? node2.children[3] : node2, 2);

                    edgeProc(
                    node1.type == DCNodeType.Internal ? node1.children[4] : node1,
                    node2.type == DCNodeType.Internal ? node2.children[0] : node2,
                    node1.type == DCNodeType.Internal ? node1.children[5] : node1,
                    node2.type == DCNodeType.Internal ? node2.children[1] : node2, 1);

                    edgeProc(
                    node1.type == DCNodeType.Internal ? node1.children[6] : node1,
                    node2.type == DCNodeType.Internal ? node2.children[2] : node2,
                    node1.type == DCNodeType.Internal ? node1.children[7] : node1,
                    node2.type == DCNodeType.Internal ? node2.children[3] : node2, 1);
                    break;
                case 1:
                    edgeProc(
                    node1.type == DCNodeType.Internal ? node1.children[2] : node1,
                    node1.type == DCNodeType.Internal ? node1.children[3] : node1,
                    node2.type == DCNodeType.Internal ? node2.children[0] : node2,
                    node2.type == DCNodeType.Internal ? node2.children[1] : node2, 0);

                    edgeProc(
                    node1.type == DCNodeType.Internal ? node1.children[6] : node1,
                    node1.type == DCNodeType.Internal ? node1.children[7] : node1,
                    node2.type == DCNodeType.Internal ? node2.children[4] : node2,
                    node2.type == DCNodeType.Internal ? node2.children[5] : node2, 0);

                    edgeProc(
                    node1.type == DCNodeType.Internal ? node1.children[2] : node1,
                    node2.type == DCNodeType.Internal ? node2.children[0] : node2,
                    node1.type == DCNodeType.Internal ? node1.children[6] : node1,
                    node2.type == DCNodeType.Internal ? node2.children[4] : node2, 2);

                    edgeProc(
                    node1.type == DCNodeType.Internal ? node1.children[3] : node1,
                    node2.type == DCNodeType.Internal ? node2.children[1] : node2,
                    node1.type == DCNodeType.Internal ? node1.children[7] : node1,
                    node2.type == DCNodeType.Internal ? node2.children[5] : node2, 2);
                    break;
                case 2:
                    edgeProc(
                    node1.type == DCNodeType.Internal ? node1.children[1] : node1,
                    node2.type == DCNodeType.Internal ? node2.children[0] : node2,
                    node1.type == DCNodeType.Internal ? node1.children[3] : node1,
                    node2.type == DCNodeType.Internal ? node2.children[2] : node2, 0);

                    edgeProc(
                    node1.type == DCNodeType.Internal ? node1.children[5] : node1,
                    node2.type == DCNodeType.Internal ? node2.children[4] : node2,
                    node1.type == DCNodeType.Internal ? node1.children[7] : node1,
                    node2.type == DCNodeType.Internal ? node2.children[6] : node2, 0);

                    edgeProc(
                    node1.type == DCNodeType.Internal ? node1.children[1] : node1,
                    node1.type == DCNodeType.Internal ? node1.children[5] : node1,
                    node2.type == DCNodeType.Internal ? node2.children[0] : node2,
                    node2.type == DCNodeType.Internal ? node2.children[4] : node2, 1);

                    edgeProc(
                    node1.type == DCNodeType.Internal ? node1.children[3] : node1,
                    node1.type == DCNodeType.Internal ? node1.children[7] : node1,
                    node2.type == DCNodeType.Internal ? node2.children[2] : node2,
                    node2.type == DCNodeType.Internal ? node2.children[6] : node2, 1);
                    break;
                default: // invalid state
                    return;
            }
        }
    }

    public void CellProc(DCNode node)
    {
        debugCellProcCount++;
        if (node == null)
            return;

        if (node.type == DCNodeType.Internal)
        {
            for (int i = 0; i < 8; i++)
            {
                var cnode = node.children[i];
                if(cnode != null) // avoid function call
                    CellProc(node.children[i]);
            }

            faceProc(node.children[0], node.children[4], 0);
            faceProc(node.children[1], node.children[5], 0);
            faceProc(node.children[2], node.children[6], 0);
            faceProc(node.children[3], node.children[7], 0);
            faceProc(node.children[0], node.children[2], 1);
            faceProc(node.children[4], node.children[6], 1);
            faceProc(node.children[1], node.children[3], 1);
            faceProc(node.children[5], node.children[7], 1);
            faceProc(node.children[0], node.children[1], 2);
            faceProc(node.children[4], node.children[5], 2);
            faceProc(node.children[2], node.children[3], 2);
            faceProc(node.children[6], node.children[7], 2);

            edgeProc(node.children[0], node.children[1], node.children[2], node.children[3], 0);
            edgeProc(node.children[4], node.children[5], node.children[6], node.children[7], 0);
            edgeProc(node.children[0], node.children[4], node.children[1], node.children[5], 1);
            edgeProc(node.children[2], node.children[6], node.children[3], node.children[7], 1);
            edgeProc(node.children[0], node.children[2], node.children[4], node.children[6], 2);
            edgeProc(node.children[1], node.children[3], node.children[5], node.children[7], 2);
        }
        
    }

    int debugProcessEdgeCount = 0;
    int debugEdgeProcCount = 0;
    int debugFaceProcCount = 0;
    int debugCellProcCount = 0;

    public void RunBench(DCNode node)
    {
        var sw = new System.Diagnostics.Stopwatch();
        sw.Start();
        for (int i = 0; i < 100; i++)
        {
            CellProc(rootNode);
        }
        sw.Stop();
        Debug.LogFormat("Time: {0}ms CellProc: {1} EdgeProc: {2} FaceProc: {3} ProcessEdge: {4}", sw.Elapsed.TotalMilliseconds, debugCellProcCount, debugEdgeProcCount, debugFaceProcCount, debugProcessEdgeCount);
    }

    public void DebugRender(DCNode node)
    {
        debugCellProcCount = 0;
        debugEdgeProcCount = 0;
        debugFaceProcCount = 0;
        debugProcessEdgeCount = 0;

        if (node == null)
            return;
        if(node.type == DCNodeType.Leaf)
        {
            //QuickShapes.DrawCubeEdges(node.minCorner + (node.size * new Vector3(0.5f, 0.5f, 0.5f)), node.size*0.9f, 0.01f, Color.red);
        }
        else
        {
            //QuickShapes.DrawCubeEdges(node.minCorner + (node.size * new Vector3(0.5f, 0.5f, 0.5f)), node.size*0.9f, 0.005f, Color.yellow);
            for(int i = 0; i < 8; i++)
            {
                DebugRender(node.children[i]);
            }
        }

        if (node == rootNode)
        {
            var sw = new System.Diagnostics.Stopwatch();
            sw.Start();
            CellProc(rootNode);
            sw.Stop();
            Debug.LogFormat("Time: {0}ms CellProc: {1} EdgeProc: {2} FaceProc: {3} ProcessEdge: {4}", sw.Elapsed.TotalMilliseconds, debugCellProcCount, debugEdgeProcCount, debugFaceProcCount, debugProcessEdgeCount);
        }
    }

    public static readonly int[,] edgevmap = new int[,]
    {
        {0,4},{1,5},{2,6},{3,7},	// x-axis 
        {0,2},{1,3},{4,6},{5,7},	// y-axis
        {0,1},{2,3},{4,5},{6,7}		// z-axis
    };

    public readonly static int[,,] faceNodeInnerEdge = new int[3, 4, 2]
    {
        { { 3, 7}, {2, 6}, {1, 5}, {0, 4} },
        { { 5, 7}, {1, 3}, {4, 6}, {0, 2} },
        { { 6, 7}, {4, 5}, {2, 3}, {0, 1} },
    };

}

