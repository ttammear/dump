using System;
using System.Collections;
using System.Collections.Generic;
using System.Text;
using System.Linq;
using UnityEngine;

public class GraphNode
{
    public float fScore; // total cost of getting to destination
    public float gScore; // cost of getting to this node
    public GraphNode cameFrom;

    public bool accessible = true;
    public bool closed = false;
    public Vector3 position;
    public List<GraphNode> neighbours;
    public Color color = Color.white;
};

public class Astar
{
    public Color notVisitedColor = Color.yellow;
    public Color closedColor = Color.black;
    public Color openColor = Color.green;
    public Color blockedColor = Color.cyan;

    public HashSet<GraphNode> openSet = new HashSet<GraphNode>();

    public Dictionary<GraphNode, GraphNode> cameFrom = new Dictionary<GraphNode, GraphNode>();
    public List<GraphNode> allNodes = new List<GraphNode>();

    public double stepWait = 1.0/240.0;

    public double stepWait2 = 1.0 / 5.0;

    const bool GRID_ALLOW_DIAGONAL = true;

    bool blueLine = false;
    Vector2 blueLineStart;
    Vector2 blueLineEnd;

    bool gline = false;
    List<Vector3> glinePath = new List<Vector3>();
    List<Vector3> heurPath = new List<Vector3>();

    float heuristicCostEstimate(GraphNode from, GraphNode to)
    {
        float dist = Vector3.Distance(from.position, to.position);
        return Mathf.Pow(dist, 1.0f);
        
        
        /*float manhattanDistance = Mathf.Abs(to.position.x - from.position.x) 
            + Mathf.Abs(to.position.y - from.position.y)
            + Mathf.Abs(to.position.z - from.position.z);
        return manhattanDistance * 1.05f; // 0.05 - tie braker*/

        //return -99999.0f;
    }

    float distBetween(GraphNode n1, GraphNode n2)
    {
        return Vector3.Distance(n1.position, n2.position);
    }

    public void GenerateGrid(Vector3 pos, float spacing, int size)
    {
        for(int i = 0; i < size; i++)
        for(int j = 0; j < size; j++)
        {
            Vector3 position = new Vector3(i*spacing, 0.0f, j*spacing) + pos;
            GraphNode newNode = new GraphNode();
            newNode.position = position;
            newNode.neighbours = new List<GraphNode>();

            newNode.accessible = !Physics.Linecast(position + new Vector3(0.0f, 10.0f, 0.0f), position, 1 << 8);
            newNode.color = newNode.accessible ? notVisitedColor : blockedColor;

            allNodes.Add(newNode);
        }
        for(int i = 0; i < size; i++)
        for(int j = 0; j < size; j++)
        {
            if(i != 0)
                allNodes[i*50 + j].neighbours.Add(allNodes[(i-1)*50 + j]);
            if(j != 0)
                allNodes[i*50 + j].neighbours.Add(allNodes[i*50 + j - 1]);
            if(i != size-1)
                allNodes[i*50 + j].neighbours.Add(allNodes[(i+1)*50 + j]);
            if(j != size-1)
                allNodes[i*50 + j].neighbours.Add(allNodes[i*50 + j + 1]);
            if(GRID_ALLOW_DIAGONAL)
            {
                if(i != 0 && j != 0)
                    allNodes[i*50 + j].neighbours.Add(allNodes[(i-1)*50 + j - 1]);
                if(i != 0 && j != size-1)
                    allNodes[i*50 + j].neighbours.Add(allNodes[(i-1)*50 + j + 1]);
                if(i != size-1 && j != 0)
                    allNodes[i*50 + j].neighbours.Add(allNodes[(i+1)*50 + j - 1]);
                if(i != size-1 && j != size-1)
                    allNodes[i*50 + j].neighbours.Add(allNodes[(i+1)*50 + j + 1]);
            }
        }
    }

    public GraphNode GetTileNode(Vector2 pos, float spacing, int size)
    {
        int tileX = Mathf.FloorToInt(pos.x / spacing);
        int tileY = Mathf.FloorToInt(pos.y / spacing);
        Debug.Log($"{tileX} {tileY}");
        if(tileX < 0 || tileY < 0 || tileX >= size || tileY >= size)
            return null;
        return allNodes[tileX * 50 + tileY];
    }

    public void Render()
    {
        foreach(var node in allNodes)
        {
            Vector3 pos3 = 10.0f * (node.position - new Vector3(25.0f, 0.0f, 25.0f));
            QuickShapes2D.DrawCircle(new Vector2(pos3.x, pos3.z), 7.5f, node.color);
            //QuickShapes.DrawSphere(node.position, 0.3f, node.color);
        }
        if(blueLine)
            QuickShapes2D.DrawLine(blueLineStart, blueLineEnd, 1.0f, Color.blue);
        if (gline)
        {
            QuickShapes2D.DrawPath(glinePath, 2.0f, Color.magenta);
            QuickShapes2D.DrawPath(heurPath, 2.0f, new Color(0.976f, 0.537f, 0.062f, 1.0f));
        }
        
    }

    public void ResetGrid()
    {
        allNodes.Clear();
        openSet.Clear();
        cameFrom.Clear();
        GenerateGrid(Vector3.zero, 1.0f, 50);
        foreach(var node in allNodes)
        {
            node.fScore = float.MaxValue;
            node.gScore = float.MaxValue;
            node.cameFrom = null;
            node.closed = false;
        }
        Debug.Log("Reset grid");
    }

    //public IEnumerator dostuff(GraphNode start, GraphNode destination, List<GraphNode> path)
    public IEnumerator dostuff(Vector2 startPos, Vector2 destPos, List<GraphNode> path)
    {
        ResetGrid();

        GraphNode start = GetTileNode(startPos, 1.0f, 50);
        GraphNode destination = GetTileNode(destPos, 1.0f, 50);

        openSet.Add(start);
        start.color = openColor;

        start.gScore = 0;
        start.fScore = heuristicCostEstimate(start, destination);

        while(openSet.Count > 0)
        {
            float currentValue = float.MaxValue;
            GraphNode current = null;
            foreach(var node in openSet)
            {
                if(node.fScore < currentValue)
                {
                    currentValue = node.fScore;
                    current = node;
                }
            }

            if(current == destination)
            {
                var rpath = reconstructPath(current);
                path.Clear();
                path.AddRange(rpath);
                Debug.Log($"Path {rpath.Count-1} hops");
                yield break;
            }

            openSet.Remove(current);
            current.closed = true;
            current.color = closedColor;
            double wait = MyTime.ConsumeDebt(stepWait);
            if(wait > 0.0)
                yield return new CustomWait(wait, true);

            if(current.neighbours == null)
                continue;
            foreach(var neigh in current.neighbours)
            {
                if(!neigh.accessible || neigh.closed)
                    continue;
                if(!openSet.Contains(neigh))
                {
                    openSet.Add(neigh);
                    neigh.color = Color.green;
                    wait = MyTime.ConsumeDebt(stepWait);
                    if(wait > 0.0)
                        yield return new CustomWait(wait, true);
                }

                float tentative_gScore = current.gScore + distBetween(current, neigh);
                // some other node already has a cheaper path to this node
                if(tentative_gScore >= neigh.gScore)
                    continue;

                //cameFrom[neigh] = current;
                neigh.cameFrom = current;
                neigh.gScore = tentative_gScore;
                neigh.fScore = tentative_gScore + heuristicCostEstimate(neigh, destination);

            }
        }
        path.Clear();
        yield break;
    }

    Vector2 nodePosTo2D(Vector3 nodePos)
    {
        return new Vector2((nodePos.x - 25.0f) * 10.0f, (nodePos.z - 25.0f) * 10.0f);
    }

    public IEnumerator doDetailedStuff(Vector2 startPos, Vector2 destPos, List<GraphNode> path, Action<string> setOpensetText)
    {
        int count = 0;

        ResetGrid();

        GraphNode start = GetTileNode(startPos, 1.0f, 50);
        GraphNode destination = GetTileNode(destPos, 1.0f, 50);

        openSet.Add(start);
        start.color = openColor;

        start.gScore = 0;
        start.fScore = heuristicCostEstimate(start, destination);

        while(openSet.Count > 0)
        {
            count++;

            Action refreshOpenset = () =>
            {
                StringBuilder sb = new StringBuilder("Open nodes ordered by fScore\n");
                var sortedArr = openSet.OrderBy(x => x.fScore).Take(15).ToArray();
                foreach (var sortedNode in sortedArr)
                {
                    sb.Append("  ");
                    if (sortedNode == sortedArr[0])
                        sb.Append("<color=yellow>");
                    else
                        sb.Append("<color=green>");
                    sb.Append(sortedNode.fScore.ToString());
                    sb.Append("</color>");
                    sb.AppendLine();
                }
                if (openSet.Count() > 15)
                    sb.Append("  ...");
                setOpensetText(sb.ToString());
            };
            refreshOpenset();

            float currentValue = float.MaxValue;
            GraphNode current = null;
            foreach(var node in openSet)
            {
                if(node.fScore < currentValue)
                {
                    currentValue = node.fScore;
                    current = node;
                }
            }
            current.color = Color.yellow;

            if(current == destination)
            {
                var rpath = reconstructPath(current);
                path.Clear();
                path.AddRange(rpath);
                Debug.Log($"Path {rpath.Count-1} hops");
                yield break;
            }

            openSet.Remove(current);
            current.closed = true;
            double wait = 0.0;
            /*wait = MyTime.ConsumeDebt(stepWait2);
            if(wait > 0.0)
                yield return new CustomWait(wait, true);*/

            if(current.neighbours == null)
                continue;
            foreach(var neigh in current.neighbours)
            {
                if(!neigh.accessible || neigh.closed)
                    continue;

                // not visible because of the magenta line (gline)
                /*blueLine = true;
                blueLineStart = new Vector2(current.position.x, current.position.z) * 10.0f - new Vector2(250.0f, 250.0f);
                blueLineEnd = new Vector2(neigh.position.x, neigh.position.z) * 10.0f - new Vector2(250.0f, 250.0f);*/
                if(!openSet.Contains(neigh))
                {
                    openSet.Add(neigh);
                    neigh.color = Color.green;
                }

                var gpath = new List<Vector3>();
                gpath.Add(nodePosTo2D(neigh.position));
                gpath.AddRange(reconstructPath(current).Select(x => new Vector3((x.position.x-25.0f)*10.0f, (x.position.z-25.0f)*10.0f, x.position.z)).ToList());
                //yield return MonoBehaviour.FindObjectOfType<MonoBehaviour>().StartCoroutine(new ProgressiveLineCo(gpath, 0.1f, Color.magenta));
                gline = true;
                glinePath = gpath;
                float tentative_gScore = current.gScore + distBetween(current, neigh);
                // some other node already has a cheaper path to this node
                if(tentative_gScore >= neigh.gScore)
                    continue;

                var fpath = new List<Vector3>();
                //fpath.AddRange(gpath.Select(x => x).Reverse().ToList());
                fpath.Add(nodePosTo2D(neigh.position));
                fpath.Add(nodePosTo2D(destination.position));
                heurPath = fpath;
                //yield return MonoBehaviour.FindObjectOfType<MonoBehaviour>().StartCoroutine(new ProgressiveLineCo(fpath, 0.3f, ));

                //cameFrom[neigh] = current;
                neigh.cameFrom = current;
                neigh.gScore = tentative_gScore;
                neigh.fScore = tentative_gScore + heuristicCostEstimate(neigh, destination);
                refreshOpenset();

                wait = MyTime.ConsumeDebt(stepWait2);
                if (wait > 0.0)
                    yield return new CustomWait(wait, true);
            }
            current.color = closedColor;
            gline = false;
        }
        path.Clear();
        yield break;
    }


    List<GraphNode> reconstructPath(GraphNode current)
    {
        List<GraphNode> ret = new List<GraphNode>();
        while(current.cameFrom != null)
        {
            ret.Add(current);
            current = current.cameFrom;
        }
        ret.Add(current);
        // NOTE: nodes are destination->start
        return ret;
    }
}
