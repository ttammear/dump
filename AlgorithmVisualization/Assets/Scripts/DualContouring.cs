using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class DualContouring : MonoBehaviour
{
    public enum OctreeNodeType
    {
         None,
         Internal,
         Psuedo,
         Leaf,
    };

    const float QEF_ERROR = 1e-6f;
    const int QEF_SWEEPS = 4;

    public class OctreeDrawInfo 
    {
        public OctreeDrawInfo(int dummy)
        {
            index = -1;
            this.corners = 0;
            position = Vector3.zero;
            averageNormal = Vector3.zero;
            qef = null;
        }

        public int index;
        public int corners;
        public Vector3 position;
        public Vector3 averageNormal;
        public QefData qef;
    };

    public class DOctreeNode
    {
        public DOctreeNode()
        {
            type = OctreeNodeType.None;
            min = new IVec3(0, 0, 0);
            size = 0;
            for (int i = 0; i < 8; i++)
                children[i] = null;
            drawInfo = null;
        }

        public DOctreeNode(OctreeNodeType type)
        {
            this.type = type;
            min = new IVec3(0, 0, 0);
            size = 0;
            for (int i = 0; i < 8; i++)
                children[i] = null;
            drawInfo = null;
        }

        public OctreeNodeType type;
        public IVec3   min;
        public int    size;
        public DOctreeNode[] children = new DOctreeNode[8];
        public OctreeDrawInfo drawInfo;
    };

    float Sphere(Vector3 worldPosition, Vector3 origin, float radius)
    {
        return (worldPosition - origin).magnitude - radius;
    }

    float Density_Func(Vector3 pos)
    {
        return Sphere(pos, Vector3.zero, 5.0f);
    }

    Vector3 CalculateSurfaceNormal(Vector3 p)
    {
        float H = 0.01f;
        float dx = Density_Func(p + new Vector3(H, 0.0f, 0.0f)) - Density_Func(p - new Vector3(H, 0.0f, 0.0f));
        float dy = Density_Func(p + new Vector3(0.0f, H, 0.0f)) - Density_Func(p - new Vector3(0.0f, H, 0.0f));
        float dz = Density_Func(p + new Vector3(0.0f, 0.0f, H)) - Density_Func(p - new Vector3(0.0f, 0.0f, H));

        QefSolver.normalize(ref dx, ref dy, ref dz);
        return new Vector3(dx, dy, dz);
    }

    DOctreeNode BuildOctree(IVec3 min, int size, float threshold)
    {
        DOctreeNode root = new DOctreeNode();
        root.min = min;
        root.size = size;
        root.type = OctreeNodeType.Internal;

        ConstructOctreeNodes(root);
        //root = SimplifyOctree(root, threshold);

        return root;
    }

    DOctreeNode ConstructOctreeNodes(DOctreeNode node)
    {
        if (node == null)
        {
            return null;
        }

        if (node.size == 1)
        {
            return ConstructLeaf(node);
        }

        int childSize = node.size / 2;
        bool hasChildren = false;
        int childCount = 0;
        DOctreeNode cnode = null;

        for (int i = 0; i < 8; i++)
        {
            DOctreeNode child = new DOctreeNode();
            child.size = childSize;
            child.min = node.min + (CHILD_MIN_OFFSETS[i] * childSize);
            child.type = OctreeNodeType.Internal;

            node.children[i] = ConstructOctreeNodes(child);
            hasChildren |= (node.children[i] != null);
            if (node.children[i] != null)
            {
                childCount++;
                cnode = node.children[i];
            }
        }

        if (childCount == 1 && cnode.type == OctreeNodeType.Leaf)
        {
            var c = ConstructLeaf(node);
            if(c != null)
                Debug.Log("1 Child!! "+node.size);
        }

        else if (!hasChildren)
        {
            return null;
        }

        return node;
    }

    DOctreeNode ConstructLeaf(DOctreeNode leaf)
    {
        if (leaf == null /*|| leaf.size != 1*/)
        {
            return null;
        }

        int corners = 0;
        for (int i = 0; i < 8; i++)
        {
            IVec3 cornerPos = leaf.min + leaf.size*CHILD_MIN_OFFSETS[i];
            float density = Density_Func(new Vector3(cornerPos.x, cornerPos.y, cornerPos.z));
            int material = density < 0.0f ? 1 : 0;
            corners |= (material << i);
        }

        if (corners == 0 || corners == 255)
        {
            return null;
        }

        // otherwise the voxel contains the surface, so find the edge intersections
        const int MAX_CROSSINGS = 6;
        int edgeCount = 0;
        Vector3 averageNormal = Vector3.zero;
        QefSolver qef = new QefSolver();

        for (int i = 0; i < 12 && edgeCount < MAX_CROSSINGS; i++)
        {
            int c1 = edgevmap[i,0];
            int c2 = edgevmap[i,1];

            int m1 = (corners >> c1) & 1;
            int m2 = (corners >> c2) & 1;

            if ((m1 == 0 && m2 == 0) ||
            (m1 == 1 && m2 == 1))
            {
                // no zero crossing on this edge
                continue;
            }

            IVec3 ip1 = leaf.min + CHILD_MIN_OFFSETS[c1];
            IVec3 ip2 = leaf.min + CHILD_MIN_OFFSETS[c2];

            Vector3 p1 = new Vector3(ip1.x, ip1.y, ip1.z);
            Vector3 p2 = new Vector3(ip2.x, ip2.y, ip2.z);
            Vector3 p = ApproximateZeroCrossingPosition(p1, p2);
            Vector3 n = CalculateSurfaceNormal(p);
            qef.Add(p.x, p.y, p.z, n.x, n.y, n.z);

            averageNormal += n;

            edgeCount++;
        }

        Vector3 qefPosition;

        qef.Solve(out qefPosition, QEF_ERROR, QEF_SWEEPS, QEF_ERROR);

        OctreeDrawInfo drawInfo = new OctreeDrawInfo(0);
        //drawInfo.position = new Vector3(qefPosition.x, qefPosition.y, qefPosition.z);
        drawInfo.position = new Vector3(leaf.min.x + 0.5f*leaf.size, leaf.min.y+0.5f*leaf.size, leaf.min.z+0.5f*leaf.size);
        drawInfo.qef = qef.GetData();

        Vector3 min = new Vector3(leaf.min.x, leaf.min.y, leaf.min.z);
        Vector3 max = min + new Vector3(leaf.size, leaf.size, leaf.size);
        if (drawInfo.position.x < min.x || drawInfo.position.x > max.x ||
        drawInfo.position.y < min.y || drawInfo.position.y > max.y ||
        drawInfo.position.z < min.z || drawInfo.position.z > max.z)
        {
            Vector3 mp = qef.massPoint;
            drawInfo.position = new Vector3(mp.x, mp.y, mp.z);
        }

        drawInfo.averageNormal = (averageNormal / (float)edgeCount);
        QefSolver.normalize(ref drawInfo.averageNormal.x, ref drawInfo.averageNormal.y, ref drawInfo.averageNormal.z);
        drawInfo.corners = corners;

        leaf.type = OctreeNodeType.Leaf;
        leaf.drawInfo = drawInfo;

        return leaf;
    }

    Vector3 ApproximateZeroCrossingPosition(Vector3 p0, Vector3 p1)
    {
        // approximate the zero crossing by finding the min value along the edge
        float minValue = float.MaxValue;
        float t = 0.0f;
        float currentT = 0.0f;
        const int steps = 8;
        const float increment = 1.0f / (float)steps;
        while (currentT <= 1.0f)
        {
            Vector3 p = p0 + ((p1 - p0) * currentT);
            float density = Mathf.Abs(Density_Func(p));
            if (density < minValue)
            {
                minValue = density;
                t = currentT;
            }
            currentT += increment;
        }

        return p0 + ((p1 - p0) * t);
    }

    DOctreeNode SimplifyOctree(DOctreeNode node, float threshold)
    {
        if (node == null)
        {
            return null;
        }

        if (node.type != OctreeNodeType.Internal)
        {
            // can't simplify!
            return node;
        }

        QefSolver qef = new QefSolver();
        int[] signs = { -1, -1, -1, -1, -1, -1, -1, -1 };
        int midsign = -1;
        int edgeCount = 0;
        bool isCollapsible = true;

        for (int i = 0; i < 8; i++)
        {
            node.children[i] = SimplifyOctree(node.children[i], threshold);
            if (node.children[i] != null)
            {
                DOctreeNode child = node.children[i];
                if (child.type == OctreeNodeType.Internal)
                {
                    isCollapsible = false;
                }
                else
                {
                    qef.Add(child.drawInfo.qef);

                    midsign = (child.drawInfo.corners >> (7 - i)) & 1; 
                    signs[i] = (child.drawInfo.corners >> i) & 1; 

                    edgeCount++;
                }
            }
        }

        if (!isCollapsible)
        {
            // at least one child is an internal node, can't collapse
            return node;
        }

        Vector3 qefPosition;
        qef.Solve(out qefPosition, QEF_ERROR, QEF_SWEEPS, QEF_ERROR);
        float error = qef.GetError();

        // convert to glm vec3 for ease of use
        Vector3 position = new Vector3(qefPosition.x, qefPosition.y, qefPosition.z);

        // at this point the masspoint will actually be a sum, so divide to make it the average
        if (error > threshold)
        {
            // this collapse breaches the threshold
            return node;
        }

        if (position.x < node.min.x || position.x > (node.min.x + node.size) ||
            position.y < node.min.y || position.y > (node.min.y + node.size) ||
            position.z < node.min.z || position.z > (node.min.z + node.size))
        {
            var mp = qef.massPoint;
            position = new Vector3(mp.x, mp.y, mp.z);
        }

        // change the node from an internal node to a 'psuedo leaf' node
        OctreeDrawInfo drawInfo = new OctreeDrawInfo(0);

        for (int i = 0; i < 8; i++)
        {
            if (signs[i] == -1)
            {
                // Undetermined, use centre sign instead
                drawInfo.corners |= (midsign << i);
            }
            else 
            {
                drawInfo.corners |= (signs[i] << i);
            }
        }

        drawInfo.averageNormal = Vector3.zero;
        for (int i = 0; i < 8; i++)
        {
            if (node.children[i] != null)
            {
                DOctreeNode child = node.children[i];
                if (child.type == OctreeNodeType.Psuedo || 
                    child.type == OctreeNodeType.Leaf)
                {
                    drawInfo.averageNormal += child.drawInfo.averageNormal;
                }
            }
        }

        QefSolver.normalize(ref drawInfo.averageNormal.x, ref drawInfo.averageNormal.y, ref drawInfo.averageNormal.z);
        //drawInfo.averageNormal = drawInfo.averageNormal.normalized;
        drawInfo.position = position;
        drawInfo.qef = qef.GetData();

        for (int i = 0; i < 8; i++)
        {
            //DestroyOctree(node.children[i]);
            node.children[i] = null;
        }

        node.type = OctreeNodeType.Psuedo;
        node.drawInfo = drawInfo;

        return node;
    }


    void GenerateVertexIndices(DOctreeNode node, List<Vector3> vertexBuffer, List<Vector3> normalBuffer)
    {
        if (node == null)
        {
            return;
        }

        if (node.type != OctreeNodeType.Leaf)
        {
            for (int i = 0; i < 8; i++)
            {
                GenerateVertexIndices(node.children[i], vertexBuffer, normalBuffer);
            }
        }

        if (node.type != OctreeNodeType.Internal)
        {
            OctreeDrawInfo d = node.drawInfo;
            if (d == null)
            {
                Debug.LogError("Error! Could not add vertex");
                throw new System.Exception();
            }

            d.index = vertexBuffer.Count;
            vertexBuffer.Add(d.position);
            normalBuffer.Add(d.averageNormal);
        }
    }

    // ----------------------------------------------------------------------------

    void ContourProcessEdge(DOctreeNode[] node, int dir, List<int> indexBuffer)
    {
        for(int i = 0; i < 4; i++)
        {
            if (node[i].size != 1)
                Debug.Log("Found");
        }

        int minSize = int.MaxValue;
        int minIndex = 0;
        int[] indices = { -1, -1, -1, -1 };
        bool flip = false;
        bool[] signChange = { false, false, false, false };

        for (int i = 0; i < 4; i++)
        {
            int edge = processEdgeMask[dir,i];
            int c1 = edgevmap[edge,0];
            int c2 = edgevmap[edge,1];

            int m1 = (node[i].drawInfo.corners >> c1) & 1;
            int m2 = (node[i].drawInfo.corners >> c2) & 1;

            if (node[i].size < minSize)
            {
                minSize = node[i].size;
                minIndex = i;
                flip = m1 != 0; 
            }

            indices[i] = node[i].drawInfo.index;

            signChange[i] = (m1 != m2);
        }

        if (signChange[minIndex])
        {
            if (!flip)
            {
                indexBuffer.Add(indices[0]);
                indexBuffer.Add(indices[1]);
                indexBuffer.Add(indices[3]);

                indexBuffer.Add(indices[0]);
                indexBuffer.Add(indices[3]);
                indexBuffer.Add(indices[2]);
            }
            else
            {
                indexBuffer.Add(indices[0]);
                indexBuffer.Add(indices[3]);
                indexBuffer.Add(indices[1]);

                indexBuffer.Add(indices[0]);
                indexBuffer.Add(indices[2]);
                indexBuffer.Add(indices[3]);
            }
        }
    }

    // ----------------------------------------------------------------------------

    void ContourEdgeProc(DOctreeNode[] node, int dir, List<int> indexBuffer)
    {
        if (node[0] == null || node[1] == null || node[2] == null || node[3] == null)
        {
            return;
        }

        if (node[0].type != OctreeNodeType.Internal &&
            node[1].type != OctreeNodeType.Internal &&
            node[2].type != OctreeNodeType.Internal &&
            node[3].type != OctreeNodeType.Internal)
        {
            ContourProcessEdge(node, dir, indexBuffer);
        }
        else
        {
            for (int i = 0; i < 2; i++)
            {
                DOctreeNode[] edgeNodes = new DOctreeNode[4];
                int[] c = 
                {
                    edgeProcEdgeMask[dir,i,0],
                    edgeProcEdgeMask[dir,i,1],
                    edgeProcEdgeMask[dir,i,2],
                    edgeProcEdgeMask[dir,i,3],
                };

                for (int j = 0; j < 4; j++)
                {
                    if (node[j].type != OctreeNodeType.Internal)
                    {
                        edgeNodes[j] = node[j];
                    }
                    else
                    {
                        edgeNodes[j] = node[j].children[c[j]];
                    }
                }

                ContourEdgeProc(edgeNodes, dir, indexBuffer);
            }
        }
    }

    // ----------------------------------------------------------------------------

    void ContourFaceProc(DOctreeNode[] node, int dir, List<int> indexBuffer)
    {
        if (node[0] == null || node[1] == null)
        {
            return;
        }

        if (node[0].type == OctreeNodeType.Internal || 
            node[1].type == OctreeNodeType.Internal)
        {
            for (int i = 0; i < 4; i++)
            {
                DOctreeNode[] faceNodes = new DOctreeNode[2];
                int[] c = 
                {
                    faceProcFaceMask[dir, i, 0], 
                    faceProcFaceMask[dir, i, 1], 
                };

                for (int j = 0; j < 2; j++)
                {
                    if (node[j].type != OctreeNodeType.Internal)
                    {
                        faceNodes[j] = node[j];
                    }
                    else
                    {
                        faceNodes[j] = node[j].children[c[j]];
                    }
                }
                // changed from ContourFaceProc(faceNodes, faceProcFaceMask[dir, i, 2]);
                ContourFaceProc(faceNodes, dir, indexBuffer);
            }
            
            int[] orders =
            {
                0, 0, 1, 1, 0, 1, 0, 1 
            };

            for (int i = 0; i < 4; i++)
            {
                DOctreeNode[] edgeNodes = new DOctreeNode[4];
                int[] c =
                {
                    faceProcEdgeMask[dir,i,1],
                    faceProcEdgeMask[dir,i,2],
                    faceProcEdgeMask[dir,i,3],
                    faceProcEdgeMask[dir,i,4],
                };

                int ooffset = faceProcEdgeMask[dir, i, 0]*4;
                //const int* order = orders[];
                for (int j = 0; j < 4; j++)
                {
                    // first node or second node?
                    var cnode = node[orders[j + ooffset]];

                    if (cnode.type != OctreeNodeType.Internal)
                    {
                        edgeNodes[j] = cnode;
                    }
                    else
                    {
                        edgeNodes[j] = cnode.children[c[j]];
                    }
                }

                ContourEdgeProc(edgeNodes, faceProcEdgeMask[dir, i, 5], indexBuffer);
            }
        }
    }

    // ----------------------------------------------------------------------------

    void ContourCellProc(DOctreeNode node, List<int> indexBuffer)
    {
        if (node == null)
        {
            return;
        }
        if (node.size != 1 && node.type == OctreeNodeType.Leaf)
            Debug.Log("WTF");

        if (node.type == OctreeNodeType.Internal)
        {
            for (int i = 0; i < 8; i++)
            {
                ContourCellProc(node.children[i], indexBuffer);
            }

            for (int i = 0; i < 12; i++)
            {
                DOctreeNode[] faceNodes = new DOctreeNode[2];
                int[] c = { edgevmap[i, 0], edgevmap[i, 1] };
                
                faceNodes[0] = node.children[c[0]];
                faceNodes[1] = node.children[c[1]];

                ContourFaceProc(faceNodes, i/4, indexBuffer); // i/4 is edge direction where 0 = X, 1 = Y, 2 = Z
            }

            for (int i = 0; i < 6; i++)
            {
                DOctreeNode[] edgeNodes = new DOctreeNode[4];
                int[] c = 
                {
                    cellProcEdgeMask[i, 0],
                    cellProcEdgeMask[i, 1],
                    cellProcEdgeMask[i, 2],
                    cellProcEdgeMask[i, 3],
                };

                for (int j = 0; j < 4; j++)
                {
                    edgeNodes[j] = node.children[c[j]];
                }

                ContourEdgeProc(edgeNodes, i / 2, indexBuffer); // i / 2 = face
            }
        }
    }

    List<Vector3> vertBuffer = new List<Vector3>();
    List<Vector3> normalBuffer = new List<Vector3>();
    List<int> indexBuffer = new List<int>();

    void GenerateMeshFromOctree(DOctreeNode node/*, VertexBuffer& vertexBuffer, IndexBuffer& indexBuffer*/)
    {
        if (node == null)
        {
            return;
        }

        vertBuffer.Clear();
        normalBuffer.Clear();
        indexBuffer.Clear();

        GenerateVertexIndices(node, vertBuffer, normalBuffer);
        ContourCellProc(node, indexBuffer);
    }

    DOctreeNode root = null;

	// Use this for initialization
	void Start () {
        root = BuildOctree(new IVec3(-16, -16, -16), 32, 0.0f);
        //root = SimplifyOctree(root, -1.0f);
        GenerateMeshFromOctree(root);
        //Debug.Log(normalBuffer.Count);
        //ConstructOctreeNodes()

        var go = new GameObject();
        var mf = go.AddComponent<MeshFilter>();
        var mesh = new Mesh();
        mesh.SetVertices(vertBuffer);
        mesh.SetNormals(normalBuffer);
        mesh.SetTriangles(indexBuffer, 0);
        mesh.RecalculateNormals();
        mesh.RecalculateBounds();
        mf.sharedMesh = mesh;
        var mr = go.AddComponent<MeshRenderer>();
        mr.sharedMaterial = new Material(Shader.Find("Standard"));
	}
	
	// Update is called once per frame
	void Update () {

        for (int i = 0; i < vertBuffer.Count; i++)
        {
            QuickShapes.DrawSphere(vertBuffer[i], 0.1f, Color.red);
        }
		
	}

    static readonly int[,] edgevmap = 
    {
        {0,4},{1,5},{2,6},{3,7},	// x-axis 
        {0,2},{1,3},{4,6},{5,7},	// y-axis
        {0,1},{2,3},{4,5},{6,7}		// z-axis
    };

    static readonly int[,,] faceProcFaceMask = new int[,,] {
        {{4,0},{5,1},{6,2},{7,3}},
        {{2,0},{6,4},{3,1},{7,5}},
        {{1,0},{3,2},{5,4},{7,6}}
    } ;

    static readonly int[,,] faceProcEdgeMask = new int[,,] {
        {{1,4,0,5,1,1},{1,6,2,7,3,1},{0,4,6,0,2,2},{0,5,7,1,3,2}},
        {{0,2,3,0,1,0},{0,6,7,4,5,0},{1,2,0,6,4,2},{1,3,1,7,5,2}},
        {{1,1,0,3,2,0},{1,5,4,7,6,0},{0,1,5,0,4,1},{0,3,7,2,6,1}}
    };

    static readonly int[,,] edgeProcEdgeMask = new int[,,] {
        {{3,2,1,0},{7,6,5,4}},
        {{5,1,4,0},{7,3,6,2}},
        {{6,4,2,0},{7,5,3,1}},
    };

    static readonly int[,] cellProcFaceMask = {{0,4,0},{1,5,0},{2,6,0},{3,7,0},{0,2,1},{4,6,1},{1,3,1},{5,7,1},{0,1,2},{2,3,2},{4,5,2},{6,7,2}} ;
    static readonly int[,] cellProcEdgeMask = {{0,1,2,3,0},{4,5,6,7,0},{0,4,1,5,1},{2,6,3,7,1},{0,2,4,6,2},{1,3,5,7,2}} ;

    static readonly int[,] processEdgeMask = {{3,2,1,0},{7,5,6,4},{11,10,9,8}};

    static readonly IVec3[] CHILD_MIN_OFFSETS =
    {
         new IVec3( 0, 0, 0 ),
         new IVec3( 0, 0, 1 ),
         new IVec3( 0, 1, 0 ),
         new IVec3( 0, 1, 1 ),
         new IVec3( 1, 0, 0 ),
         new IVec3( 1, 0, 1 ),
         new IVec3( 1, 1, 0 ),
         new IVec3( 1, 1, 1 )
    };

}
