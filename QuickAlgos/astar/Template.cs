using System;
using AnimLib;
using System.Collections.Generic;
using System.Threading.Tasks;
using System.Linq;

public class GraphNode {
    public VisualEntity entity;
    public GraphNode[] neighbours;
}

public struct CellId {
    public int x, y;

    public override bool Equals(object obj) {
        if(!(obj is CellId))
            return false;
        CellId cid = (CellId)obj;
        return cid.x == x && cid.y == y;
    }
    public override int GetHashCode() {
        return (x*13)^(y*27);
    }
    public static bool operator ==(CellId obj1, CellId obj2)
    {
        return obj1.x == obj2.x && obj1.y == obj2.y;
    }
    public static bool operator !=(CellId obj1, CellId obj2)
    {
        return !(obj1 == obj2);
    }
}

public class Grid {
    public int width, height;
    Circle [,] circles;
    bool [,] map;
    public Grid(int width, int height) {
        this.width = width;
        this.height = height;
        circles = new Circle[width,height];
        map = new bool[width,height];
    }
    public bool isBlocked(CellId x) {
        if(x.x >= 0 && x.x < width && x.y >= 0 && x.y < height) {
            return map[x.x, x.y];
        }
        return true;
    }
    public void CreateGrid(World world, Vector2 ofst, float spacing, float r) {
        for(int j = 0; j < height; j++)
        for(int i = 0; i < width; i++)
        {
            var c = world.CreateCircle(r, Color.WHITE);
            c.Transform.Pos = (Vector3)(new Vector2(i*spacing, j*spacing) + ofst);
            circles[i,j] = c;
        }
    }

    public void SetColor(CellId x, Color c) {
        circles[x.x, x.y].Color = c;
    }

    public Vector2 GetPosition(CellId id) {
        return (Vector2)circles[id.x, id.y].Transform.Pos;
    }

    public void Reset() {
        foreach(var c in circles) {
            c.Color = Color.WHITE;
        }
    }

    public void SetFromTexture(Texture2D tex) {
        int pixSize = tex.Format == Texture2D.TextureFormat.BGR8 ? 3 : 4;
        for(int j = 0; j < height; j++)
        for(int i = 0; i < width; i++) {
            if(tex.GetPixel(i, j).R == 0) {
                map[i, 19-j] = true;
                circles[i,19-j].Color = new Color(100, 100, 100, 255);
            } else {
                map[i, 19-j] = false;
                circles[i,19-j].Color = Color.WHITE;
            }
        }
    }
}

public class Template : AnimationBehaviour
{
    const float CIRCLE_RADIUS = 0.2f;

    public void Init(AnimationSettings settings) {
        settings.Name = "AnimTest";
        settings.MaxLength = 480.0f;
    }

    double hDist(CellId start, CellId end) {
        var xdif = start.x - end.x;
        var ydif = start.y - end.y;
        return Math.Sqrt(xdif*xdif + ydif*ydif);
    }

    double hDist2(CellId start, CellId end) {
        var xdif = start.x - end.x;
        var ydif = start.y - end.y;
        return xdif*xdif + ydif*ydif;
    }

    double hDist5(CellId start, CellId end) {
        var xdif = start.x - end.x;
        var ydif = start.y - end.y;
        return Math.Sqrt(xdif*xdif + ydif*ydif)*10.0f;
    }

    double hManDist(CellId start, CellId end) {
        var xdif = start.x - end.x;
        var ydif = start.y - end.y;
        return Math.Abs(xdif) + Math.Abs(ydif);
    }

    double hManDist2(CellId start, CellId end) {
        var xdif = start.x - end.x;
        var ydif = start.y - end.y;
        return (Math.Abs(xdif) + Math.Abs(ydif))*(Math.Abs(xdif) + Math.Abs(ydif));
    }

    public async Task<List<CellId>> AstarReconstructPath(Dictionary<CellId, CellId> cameFrom, CellId from, Func<CellId, CellId, double> cost) {
        List<CellId> path = new List<CellId>();
        CellId? c = from;
        (CellId, Arrow2D) val;
        while(c != null) {
            len++;
            travelCost += cost(c.Value, cameFrom.ContainsKey(c.Value) ? cameFrom[c.Value] : c.Value);
            path.Add(c.Value);
            if(_arrowDic.TryGetValue(c.Value, out val)) {
                val.Item2.StartColor = Color.BLUE;
                val.Item2.EndColor = Color.BLUE;
            }
            c = cameFrom.ContainsKey(c.Value) ? cameFrom[c.Value] : (CellId?)null;
            await AnimationTime.WaitSeconds(0.05);
            //await AnimationTime.WaitFrame();
        }
        path.Reverse();
        return path;
    }

    // 4-way
    /*public CellId[] getNeighs(CellId cellId, Grid grid) {
        List<CellId> cellIds = new List<CellId>();
        var l = new CellId {
            x = cellId.x - 1,
            y = cellId.y
        };
        var r = new CellId {
            x = cellId.x + 1,
            y = cellId.y
        };
        var u = new CellId {
            x = cellId.x,
            y = cellId.y - 1
        };
        var d = new CellId {
            x = cellId.x,
            y = cellId.y + 1
        };
        return (new CellId[] {l, r, u , d}).Where(x => !grid.isBlocked(x)).ToArray();
    }*/

    // 8-way
    public CellId[] getNeighs(CellId cellId, Grid grid) {
        List<CellId> cellIds = new List<CellId>();
        var l = new CellId {
            x = cellId.x - 1,
            y = cellId.y
        };
        var r = new CellId {
            x = cellId.x + 1,
            y = cellId.y
        };
        var u = new CellId {
            x = cellId.x,
            y = cellId.y - 1
        };
        var d = new CellId {
            x = cellId.x,
            y = cellId.y + 1
        };
        var lu = new CellId {
            x = cellId.x - 1,
            y = cellId.y - 1
        };
        var ru = new CellId {
            x = cellId.x + 1,
            y = cellId.y - 1
        };
        var ld = new CellId {
            x = cellId.x - 1,
            y = cellId.y + 1
        };
        var rd = new CellId {
            x = cellId.x + 1,
            y = cellId.y + 1
        };
        return (new CellId[] {l, r, u , d, lu, ru, ld, rd}).Where(x => !grid.isBlocked(x)).ToArray();
    }

    static World _world;

    List<Arrow2D> _arrows = new List<Arrow2D>();
    Dictionary<CellId, (CellId, Arrow2D)> _arrowDic = new Dictionary<CellId, (CellId, Arrow2D)>();

    int count = 1;

    

    public void AddArrow(Grid grid, CellId startc, CellId endc) {
        var start = grid.GetPosition(startc);
        var end = grid.GetPosition(endc);
        Arrow2D a;
        Vector2 startp = start+(end-start)*0.1f;
        Vector2 endp = end-(end-start)*0.15f;
        if(_arrowDic.ContainsKey(startc)) {
            a = _arrowDic[startc].Item2;
            a.StartPoint = (Vector3)startp;
            a.EndPoint = (Vector3)endp;
        } else {
            a = _world.CreateArrow(startp, endp, 0.08f, Color.RED, Color.RED, -0.08f + (count++*0.00001f));
            _arrows.Add(a);
            _arrowDic.Add(startc, (endc, a));
        }
    }

    public void ClearArrows() {
        foreach(var a in _arrows) {
            _world.Destroy(a);
        }
        _arrows.Clear();
        _arrowDic.Clear();
    }

    public async Task<List<CellId>> Dijkstra(CellId start, CellId goal, Grid grid, Func<CellId, CellId, double> cost) {
        var dist = new Dictionary<CellId, double>();
        dist.Add(start, 0.0);
        var prev = new Dictionary<CellId, CellId>();
        var vertices = new HashSet<CellId>();
        for(int i = 0; i < grid.height; i++) {
            for(int j = 0; j < grid.width; j++) {
                var cid = new CellId() {
                    x = j, y = i,
                };
                if(!grid.isBlocked(cid)) {
                    vertices.Add(cid);
                }
            }
        }
        while(vertices.Count > 0) {
            double dummy;
            var min = vertices.OrderBy(x => dist.TryGetValue(x, out dummy) ? dist[x] : double.MaxValue).First();
            vertices.Remove(min);

            if(min == goal) {
                grid.SetColor(min, Color.ORANGE);
                return await AstarReconstructPath(prev, goal, cost);
            }

            grid.SetColor(min, Color.YELLOW);

            foreach(var n in getNeighs(min, grid)) {
                var alt = (dist.TryGetValue(min, out dummy) ? dist[min] : double.MaxValue) + cost(min, n);
                if(alt < (dist.TryGetValue(n, out dummy) ? dist[n] : double.MaxValue)) {
                    dist[n] = alt;
                    prev[n] = min;
                    AddArrow(grid, n, min);
                }
                await AnimationTime.WaitFrame();
            }
            grid.SetColor(min, Color.GREEN);
            await AnimationTime.WaitFrame();
        }
        return null;
    }

    public async Task<List<GraphNode>> AstarGraph(GraphNode start, GraphNode goal) {
        var openSet = new HashSet<GraphNode>();
        var cameFrom = new Dictionary<GraphNode, GraphNode>();
        var gScore = new Dictionary<GraphNode, double>();
        gScore.Add(start, 0.0);
        var fScore = new Dictionary<GraphNode, double>();
        Func<GraphNode,GraphNode,double> h = (g1, g2) => {
            return (g1.entity.state.position-g2.entity.state.position).Length;
        };
        fScore.Add(start, h(start, goal));
        openSet.Add(start);
        Func<GraphNode, List<GraphNode>> reconstructPath = (GraphNode end) => {
            var path = new List<GraphNode>();
            GraphNode c = end;
            while(c != null) {
                len++;
                path.Add(c);
                c = cameFrom.ContainsKey(c) ? cameFrom[c] : null;
                //await AnimationTime.WaitSeconds(0.05);
                //await AnimationTime.WaitFrame();
            }
            path.Reverse();
            return path;
        };
        while(openSet.Count > 0) {
            var cur = openSet.OrderBy(x => fScore.ContainsKey(x) ? fScore[x] : Double.MaxValue).First();
            if(cur == goal) {
                return reconstructPath(cur);
            }
            openSet.Remove(cur);
            foreach(var neigh in cur.neighbours) {
                var tgScore = gScore[cur] + (cur.entity.state.position-neigh.entity.state.position).Length;
                if(tgScore < (gScore.ContainsKey(neigh) ? gScore[neigh] : double.MaxValue)) {
                    cameFrom[neigh] = cur;
                    gScore[neigh] = tgScore;
                    fScore[neigh] = tgScore + h(neigh, goal);
                    if(!openSet.Contains(neigh)) {
                        openSet.Add(neigh);
                        //(neigh.entity as Circle).Color = Color.GREEN;
                    }
                }
            }
        }
        return new List<GraphNode>();
    }

    public async Task<List<CellId>> Astar(CellId start, CellId goal, Grid grid, Func<CellId, CellId, double> h, Func<CellId, CellId, double> cost) {
        var openSet = new HashSet<CellId>();

        var closedSet = new HashSet<CellId>();
        var cameFrom = new Dictionary<CellId, CellId>();

        var gScore = new Dictionary<CellId, double>();
        gScore.Add(start, 0.0);

        var fScore = new Dictionary<CellId, double>();
        fScore.Add(start, h(start, goal));

        opened = 1;

        Action<CellId> openset = (x) => {
            openSet.Add(x);
            grid.SetColor(x, Color.CYAN);
        };
        Action<CellId> closedset = (x) => {
            closedSet.Add(x);
            grid.SetColor(x, Color.GREEN);
            closed++;
        };
        openset(start);

        while(openSet.Count > 0) {
            var current = openSet.OrderBy(x => fScore.ContainsKey(x) ? fScore[x] : double.MaxValue).First();
            if(current == goal) {
                grid.SetColor(current, Color.ORANGE);
                grid.SetColor(start, Color.ORANGE);
                return await AstarReconstructPath(cameFrom, current, cost);
            }


            closedset(current);
            openSet.Remove(current);

            grid.SetColor(current, Color.YELLOW);

            foreach(var n in getNeighs(current, grid)) {
                opened++;
                var tgScore = gScore[current] + cost(current, n);
                if(tgScore < (gScore.ContainsKey(n) ? gScore[n] : double.MaxValue)) {
                    AddArrow(grid, n, current);
                    cameFrom[n] = current;
                    gScore[n] = tgScore;
                    fScore[n] = gScore[n] + h(n, goal);
                    if(!closedSet.Contains(n))
                        openset(n);
                }
                await AnimationTime.WaitFrame();
            }

            grid.SetColor(current, Color.GREEN);

            await AnimationTime.WaitFrame();
        }
        return null;
    }

    int opened = 0;
    int closed = 0;
    int len = 0;
    double travelCost = 0.0;

    void Reset(Grid grid, Texture2D tex) {
        ClearArrows();
        len = 0;
        closed = 0;
        opened = 0;
        travelCost = 0.0;
        grid.SetFromTexture(tex);
    }

    Vector3 camPos2;
    Vector3 camPos3;
    Text2D mazeExitText, mazeEntText;
    Arrow2D mazeExitArr, mazeEntArr;
    Texture2D labTex;
    TexRect maze;

    Circle[] concNodes;
    Text2D[] concTexts;
    Text2D[] concfs;
    Text2D[] concgs;
    Text2D concfbuf, concgbuf, concCurNode, concfbuf2, concgbuf2;
    Text2D titleText, titleText2, titleText3;
    Text2D[] ndnrs;
    Arrow2D concArrow31, concArrow21, concArrow43, concArrow54, concArrow64, concArrow42;

    void CreateScene(World world, AnimationPlayer player) {
        titleText = player.GetSceneEntityByName<Text2D>("titletext");
        titleText2 = player.GetSceneEntityByName<Text2D>("titletext2");
        titleText2.Active = false;
        titleText3 = player.GetSceneEntityByName<Text2D>("titletext3");
        titleText3.Active = false;
        camPos2 = player.CreateHandle3D("camPosLaby", world.ActiveCamera.state.position+new Vector3(-1.0f, 0.0f, 0.0f));
        labTex = player.GetTextureResource("maze25");
        maze = world.CreateTexturedRectangle(10.0f, 10.0f, labTex);
        maze.Transform.Pos = new Vector3(camPos2.x-7.0f, camPos2.y, 0.0f);
        mazeExitText = player.GetSceneEntityByName<Text2D>("mazeExitText");
        mazeExitText.Active = false;
        mazeEntText = player.GetSceneEntityByName<Text2D>("mazeEntText");
        mazeEntText.Active = false;
        mazeExitArr = player.GetSceneEntityByName<Arrow2D>("mazeExitArr");
        mazeExitArr.Active = false;
        mazeEntArr = player.GetSceneEntityByName<Arrow2D>("mazeEntArr");
        mazeEntArr.Active = false;
        camPos3 = player.CreateHandle3D("camPosConc", world.ActiveCamera.state.position+new Vector3(-2.0f, 0.0f, 0.0f));
        concNodes = player.GetSceneEntitiesByName<Circle>("cnd");
        System.Diagnostics.Debug.Assert(concNodes.Length == 6);
        concTexts = player.GetSceneEntitiesByName<Text2D>("concText[0-9]");
        foreach(var concText in concTexts)
            concText.Active = false;
        concfs = player.GetSceneEntitiesByName<Text2D>("conc[0-9]f");
        foreach(var concf in concfs) {
            concf.Active = false;
            concf.Text = "∞";
        }
        concgs = player.GetSceneEntitiesByName<Text2D>("conc[0-9]g");
        foreach(var concg in concgs) {
            concg.Active = false;
            concg.Text = "∞";
        }

        ndnrs = player.GetSceneEntitiesByName<Text2D>("ndnr*");
        foreach(var nd in ndnrs) {
            nd.Active = false;
        }
        concArrow31 = player.GetSceneEntityByName<Arrow2D>("concArrow31");
        concArrow31.Active = false;
        concArrow21 = player.GetSceneEntityByName<Arrow2D>("concArrow21");
        concArrow21.Active = false;
        concArrow43 = player.GetSceneEntityByName<Arrow2D>("concArrow43");
        concArrow43.Active = false;
        concArrow54 = player.GetSceneEntityByName<Arrow2D>("concArrow54");
        concArrow54.Active = false;
        concArrow64 = player.GetSceneEntityByName<Arrow2D>("concArrow64");
        concArrow64.Active = false;
        concArrow42 = player.GetSceneEntityByName<Arrow2D>("concArrow42");
        concArrow42.Active = false;

        concfbuf = player.GetSceneEntityByName<Text2D>("concfbuf");
        concfbuf.Active = false;
        concgbuf = player.GetSceneEntityByName<Text2D>("concgbuf");
        concgbuf.Active = false;
        concfbuf2 = player.GetSceneEntityByName<Text2D>("concfbuf2");
        concfbuf2.Active = false;
        concgbuf2 = player.GetSceneEntityByName<Text2D>("concgbuf2");
        concgbuf2.Active = false;
        concCurNode = player.GetSceneEntityByName<Text2D>("concCurNode");
        concCurNode.Active = false;
    }

    public async Task Animation(World world, AnimationPlayer player) {
        _world = world;
        opened = 0;
        closed = 0;
        var rnd = new Random();

        CreateScene(world, player);

        var startCamPos = world.ActiveCamera.state.position;

        var camPos = player.CreateHandle3D("camPosGraph", world.ActiveCamera.state.position+new Vector3(-1.0f, 0.0f, 0.0f));
        world.ActiveCamera.Transform.Pos = camPos;

        var nodes = new (Circle,Task)[12];
        int i;
        for(i = 0; i < nodes.Length; i++) {
            var cir = player.GetSceneEntityByName<Circle>("nd"+(i+1));
            nodes[i].Item1 = cir;
            cir.Active = false;
        }
        var nodeText = player.GetSceneEntityByName<Text2D>("nodetext");
        var edgeText = player.GetSceneEntityByName<Text2D>("edgetext");
        var weightText = player.GetSceneEntityByName<Text2D>("weighttext");
        nodeText.Active = false;
        edgeText.Active = false;
        weightText.Active = false;

        // GRAPH EXPLANATION ANIMATION STARTS HERE
        await AnimationTime.WaitSeconds(2.0);
        nodeText.Active = true;

        for(i = 0; i < nodes.Length; i++) {
            var cir = nodes[i].Item1;
            Func<Task> test = async () => {
                await AnimationTime.WaitSeconds(rnd.NextDouble()*0.2);
                cir.Active = true;
                await AnimationTransform.BouncyT<float>((float x) => {
                    cir.Radius = x;
                }, 0.0f, cir.Radius, 1.0 + rnd.NextDouble()*0.1);
            };
            nodes[i].Item2 = test();
        }
        foreach(var pair in nodes) {
            await pair.Item2;
        }
        var edges = new (Circle, Circle, SolidLine)[12] {
            (nodes[0].Item1, nodes[1].Item1, null),
            (nodes[1].Item1, nodes[2].Item1, null),
            (nodes[2].Item1, nodes[3].Item1, null),
            (nodes[3].Item1, nodes[4].Item1, null),
            (nodes[4].Item1, nodes[5].Item1, null),
            (nodes[2].Item1, nodes[5].Item1, null),
            (nodes[1].Item1, nodes[6].Item1, null),
            (nodes[6].Item1, nodes[7].Item1, null),
            (nodes[7].Item1, nodes[8].Item1, null),
            (nodes[8].Item1, nodes[9].Item1, null),
            (nodes[9].Item1, nodes[10].Item1, null),
            (nodes[9].Item1, nodes[11].Item1, null),
        };

        edgeText.Active = true;

        i = 0;
        foreach(var edge in edges) {
            var line = world.CreateSolidLine(edge.Item1.state.position.ZOfst(0.01f), edge.Item2.state.position.ZOfst(0.01f), 0.1f, Color.BLACK);
            edges[i].Item3 = line;
            var task = AnimationTransform.SmoothT<float>((float x) => {
                line.Progression = x;
            }, 0.0f, 1.0f, 0.1f);
            await task;
            i++;
        }

        weightText.Active = true;
        
        i = 0;
        foreach(var edge in edges) {
            var dist = (edge.Item1.state.position - edge.Item2.state.position).Length;
            var label = world.CreateLabel(String.Format("{0:0.00}", dist), 14.0f, edges[i].Item3);
            //label.Color = Color.RED;
            _ = AnimationTransform.BouncyFloat((float x) => {
                label.Color = Color.Lerp(Color.WHITE, new Color(50, 0, 200, 255), x);
            }, 0.0f, 1.0f, 1.0);
            i++;
        }
        await AnimationTime.WaitSeconds(1.0);
        await AnimationTime.WaitSeconds(3.0);
        await AnimationTransform.SmoothColor(titleText, Color.WHITE, 0.5);
        world.Destroy(titleText);


        // GRAPH LABYRINTH EXAMPLE

        nodeText.Active = false;
        edgeText.Active = false;
        weightText.Active = false;

        await AnimationTransform.SmoothT<Vector3>((Vector3 pos) => {
            world.ActiveCamera.Transform.Pos = pos;
        }, world.ActiveCamera.Transform.Pos, camPos2, 2.0);

        titleText2.Active = true;
        await AnimationTransform.SmoothColor(titleText2, Color.WHITE, Color.BLACK, 0.5);

        mazeEntText.Active = true;
        mazeExitText.Active = true;
        mazeEntArr.Active = true;
        mazeExitArr.Active = true;
        var col11 = mazeEntText.Color;
        var col21 = mazeExitText.Color;
        await AnimationTransform.SmoothFloat((float x) => {
            mazeEntArr.Width = x*0.2f;
            mazeExitArr.Width = x*0.2f;
            mazeEntText.Color = Color.Lerp(Color.WHITE, col11, x);
            mazeExitText.Color = Color.Lerp(Color.WHITE, col21, x);
        }, 0.0f, 1.0f, 1.0);

        await AnimationTime.WaitSeconds(1.0);

        // create maze nodes

        List<(SolidLine,Vector3,Vector3)> mazeLines = new List<(SolidLine,Vector3,Vector3)>();
        GraphNode[,] mazeNodes = new GraphNode[25,25];
        //GraphNode mazeStart, mazeEnd;

        List<(int,int)> ns = new List<(int, int)>();
        Func<(int,int),List<(int,int)>> neighs = ((int x, int y) pixc) => {
            ns.Clear();
            if(pixc.x != 24) {
                bool found = false;
                for(int pxofst = 0; pxofst < 42; pxofst++) {
                    int x = 32+(int)((float)pixc.x*42.4116f)+pxofst;
                    int y = 32+(int)((float)pixc.y*42.4116f);
                    var ppp = labTex.GetPixel(x, labTex.Height - y);
                    if(ppp.r > 100) {
                        found = true;
                        break;
                    }
                }
                if(!found)
                    ns.Add((pixc.x+1, pixc.y));
            }
            if(pixc.y != 24) {
                bool found = false;
                for(int pxofst = 0; pxofst < 42; pxofst++) {
                    int x = 32+(int)((float)pixc.x*42.4116f);
                    int y = 32+(int)((float)pixc.y*42.4116f)+pxofst;
                    var ppp = labTex.GetPixel(x, labTex.Height - y);
                    if(ppp.r > 100) {
                        found = true;
                        break;
                    }
                }
                if(!found)
                    ns.Add((pixc.x, pixc.y+1));
            }
            return ns;
        };
        (Circle,Vector3)[,] mazeCircles = new (Circle,Vector3)[25,25];
        for(int mi = 0; mi < 25; mi++){ 
            for(int mj = 0; mj < 25; mj++) {
                //var pix = labTex.GetPixel(30+42*mi, 2112 - 42*mj);
                var mazePivot = (Vector2)maze.Transform.Pos - new Vector2(5.0f, 5.0f);
                var mazeNode = world.CreateCircle(0.08f, Color.RED);
                mazeNode.Transform.Pos = (Vector3)mazePivot + new Vector3(mi*0.392f+0.28f, mj*0.392f+0.28f, -0.01f);
                mazeCircles[mi, mj] = (mazeNode, mazeNode.Transform.Pos);
                mazeNodes[mi, mj] = new GraphNode() {
                    entity = mazeNode,
                    neighbours = new GraphNode[0],
                };
            }
        }
        await AnimationTime.WaitSeconds(1.0);
        for(int mi = 0; mi < 25; mi++){ 
            for(int mj = 0; mj < 25; mj++) {
                var mazeNode = mazeCircles[mi, mj];
                var node = new GraphNode();
                var cns = neighs((mi, mj));
                var newConnections = new GraphNode[cns.Count];
                int ni = 0;
                foreach(var n in cns) {
                    var startv = mazeNode.Item1.Transform.Pos.ZOfst(0.005f);
                    var endv = mazeCircles[n.Item1, n.Item2].Item1.Transform.Pos.ZOfst(0.005f);
                    var line = world.CreateSolidLine(startv, endv, 0.11f, Color.VIOLET);
                    mazeLines.Add((line, startv, endv));

                    newConnections[ni] = mazeNodes[n.Item1, n.Item2];
                    mazeNodes[n.Item1, n.Item2].neighbours = mazeNodes[n.Item1, n.Item2].neighbours.Concat(new GraphNode[1]{mazeNodes[mi,mj]}).ToArray();
                    ni++;
                }
                mazeNodes[mi,mj].neighbours = mazeNodes[mi,mj].neighbours.Concat(newConnections).ToArray();
            }
        }
        await AnimationTransform.SmoothFloat((float x) => {
            foreach(var mline in mazeLines) {
                mline.Item1.Points = new Vector3[] {mline.Item2+x*new Vector3(12.0f, 0.0f, 0.0f), mline.Item3+x*new Vector3(12.0f, 0.0f, 0.0f)};
            }
            foreach(var c in mazeCircles) {
                c.Item1.Transform.Pos = c.Item2 + x*new Vector3(12.0f, 0.0f, 0.0f);
            }
        }, 0.0f, 1.0f, 2.0f);

        var path = await AstarGraph(mazeNodes[0,0], mazeNodes[24,24]);
        for(int pi = 0; pi < path.Count-1; pi++) {
            var startp = path[pi];
            var endp = path[pi+1];
            var startv = startp.entity.state.position.ZOfst(0.004f);
            var endv = endp.entity.state.position.ZOfst(0.004f);
            var line = world.CreateSolidLine(startv, endv, 0.11f, Color.ORANGE);
            await AnimationTransform.LerpFloat((float x) => {
                line.Points = new Vector3[2]{startv, startv+x*(endv-startv)};
            }, 0.0f, 1.0f, 0.01f);
        }

        await AnimationTime.WaitSeconds(3.0);
        mazeEntText.Active = false;
        mazeExitText.Active = false;
        await AnimationTransform.SmoothColor(titleText2, Color.WHITE, 0.5);
        world.Destroy(titleText2);

        var concEdges = new (Circle, Circle)[6] {
            (concNodes[0], concNodes[2]),
            (concNodes[0], concNodes[1]),
            (concNodes[1], concNodes[3]),
            (concNodes[3], concNodes[4]),
            (concNodes[3], concNodes[5]),
            (concNodes[2], concNodes[3]),
        };
        var concEdgeLines = new SolidLine[6];
        var concEdgeLabels = new Label[6];
        var li = 0;
        foreach(var edge in concEdges) {
            var startp = edge.Item1.Transform.Pos.ZOfst(0.001f);
            var endp = edge.Item2.Transform.Pos.ZOfst(0.001f);
            var line = world.CreateSolidLine(startp, endp, 0.1f, Color.BLACK);
            concEdgeLines[li] = line;
            var label = world.CreateLabel($"{(startp-endp).Length:0.00}", 24.0f, line);
            label.Color = new Color(33, 33, 123, 255);
            concEdgeLabels[li] = label;
            li++;
        }

        await AnimationTransform.SmoothT<Vector3>((Vector3 pos) => {
            world.ActiveCamera.Transform.Pos = pos;
        }, world.ActiveCamera.Transform.Pos, camPos3, 2.0);

        titleText3.Active = true;
        await AnimationTransform.SmoothColor(titleText3, Color.WHITE, Color.BLACK, 0.5);

        // ASTAR CONCEPTS
        concTexts[2].Active = true;
        _ = AnimationTransform.SmoothColor(concTexts[2], Color.WHITE, concTexts[2].Color, 1.0f);
        concTexts[3].Active = true;
        _ = AnimationTransform.SmoothColor(concTexts[3], Color.WHITE, concTexts[3].Color, 1.0f);
        concTexts[0].Active = true;
        await AnimationTransform.SmoothColor(concTexts[0], Color.WHITE, concTexts[0].Color, 1.0f);
        concTexts[1].Active = true;
        await AnimationTransform.SmoothColor(concTexts[1], Color.WHITE, concTexts[1].Color, 1.0f);

        // d() function
        // h() function
        var hLStart = concNodes[1].Transform.Pos.ZOfst(0.0005f);
        var hLEnd = concNodes[5].Transform.Pos.ZOfst(0.0005f);
        var hLine = world.CreateSolidLine(hLStart, hLEnd, 0.15f, new Color(123, 33, 33, 255));
        var hLabel = world.CreateLabel($"0.00", 24.0f, hLine);
        hLabel.Color = new Color(123, 33, 33, 255);
        await AnimationTransform.SmoothFloat((x) => {
            hLabel.Text = $"{(hLStart-hLEnd).Length*x:0.00}";
            hLine.Progression = x;
        }, 0.0f, 1.0f, 2.0f);
        hLStart = concNodes[2].Transform.Pos.ZOfst(0.0005f);
        await AnimationTime.WaitSeconds(1.0);
        hLine.Points = new Vector3[]{hLStart, hLEnd};
        hLine.Progression = 0.0f;
        hLabel.Text = "0.00";
        await AnimationTransform.SmoothFloat((x) => {
            hLabel.Text = $"{(hLStart-hLEnd).Length*x:0.00}";
            hLine.Progression = x;
        }, 0.0f, 1.0f, 2.0f);

        await AnimationTime.WaitSeconds(2.0);
        hLabel.Text = "";
        hLine.Progression = 0.0f;


        concTexts[4].Active = true;
        await AnimationTransform.SmoothColor(concTexts[4], Color.WHITE, concTexts[4].Color, 1.0f);
        concTexts[5].Active = true;
        await AnimationTransform.SmoothColor(concTexts[5], Color.WHITE, concTexts[5].Color, 1.0f);
        await AnimationTime.WaitSeconds(1.0);

        // gScore and fScore examples
        foreach(var concf in concfs)
            concf.Active = true;
        foreach(var concg in concgs)
            concg.Active = true;
        var fCol = concfs[0].Color;
        var gCol = concgs[0].Color;
        _ = AnimationTransform.SmoothFloat((x) => {
            foreach(var concf in concfs)
                concf.Color = Color.Lerp(Color.WHITE, fCol, x);
            foreach(var concg in concgs)
                concg.Color = Color.Lerp(Color.WHITE, gCol, x);
        }, 0.0f, 1.0f, 0.5);
        var travelCircle = world.CreateCircle(concNodes[0].Radius*0.5f, Color.RED);
        travelCircle.Transform.Pos = concNodes[0].Transform.Pos.ZOfst(-0.0001f);

        await AnimationTime.WaitSeconds(1.0);

        Func<int, Text2D, Text2D, Text2D, Label, string, Task> calcScores1 = async (nodeidx, gtext, ftext, prevNode, edgeLabel, nodename) => {
            concCurNode.Text = "Current node: " + nodename;
            gtext.Text = "0.00";
            ftext.Text = "0.00";
            await AnimationTime.WaitSeconds(1.0);
            if(prevNode != null) {
                var prevT = world.Clone<Text2D>(prevNode);
                var val = prevT.Text;
                await world.AbsorbDestroy(prevT, 1.0f, gtext.state.position.ZOfst(-0.0012f), true);
                gtext.Text = val;
            }
            if(edgeLabel != null) {
                var edgeL = world.Clone<Label>(edgeLabel);
                var val = edgeL.Text;
                await world.AbsorbDestroy(edgeL, 1.0f, gtext.state.position, true);
                gtext.Text = $"{float.Parse(gtext.Text)+float.Parse(val):0.00}";
            }
            // gscore -> fscore
            var gcop = world.Clone<Text2D>(gtext);
            await world.AbsorbDestroy(gcop, 1.0f, ftext.state.position.ZOfst(-0.0005f), true);
            ftext.Text = gtext.Text;
            await AnimationTime.WaitSeconds(0.5);
            hLStart = concNodes[nodeidx].Transform.Pos.ZOfst(0.0005f);
            hLEnd = concNodes[5].Transform.Pos.ZOfst(0.0005f);
            hLine.Points = new Vector3[] {hLStart, hLEnd};
            hLine.Progression = 0.0f;
            hLabel.Text = "0.00";
            if(nodeidx == 3) {
                concEdgeLabels[4].Active = false;
            }
            await AnimationTransform.SmoothFloat((x) => {
                hLabel.Text = $"{(hLStart-hLEnd).Length*x:0.00}";
                hLine.Progression = x;
            }, 0.0f, 1.0f, 1.0f);
            await AnimationTime.WaitSeconds(0.5);
            var test1 = world.Clone<Label>(hLabel);
            if(nodeidx == 3) {
                concEdgeLabels[4].Active = true;
            }
            await world.AbsorbDestroy(test1, 1.0f, ftext.state.position+new Vector3(50.0f, 15.0f, -0.001f), true);
            ftext.Text = $"{float.Parse(ftext.Text)+float.Parse(hLabel.Text):0.00}";
            hLabel.Text = "";
            hLine.Progression = 0.0f;
        };

        Func<int, int, Task> travelTo = async (from, to) => {
            travelCircle.Active = true;
            travelCircle.Transform.Pos = concNodes[from].Transform.Pos.ZOfst(-0.001f);
            var startPos = travelCircle.Transform.Pos;
            await AnimationTransform.SmoothFloat(x => {
                travelCircle.Transform.Pos = Vector3.Lerp(startPos, concNodes[to].Transform.Pos.ZOfst(-0.001f), x);
            }, 0.0f, 1.0f, 0.5f);
        };

        await travelTo(0, 0);
        await calcScores1(0, concfs[0], concgs[0], null, null, "1");
        await travelTo(0, 2);
        await calcScores1(2, concfs[2], concgs[2], concfs[0], concEdgeLabels[0], "2");
        await travelTo(0, 1);
        await calcScores1(1, concfs[1], concgs[1], concfs[0], concEdgeLabels[1], "3");
        await travelTo(1, 3);
        await calcScores1(3, concfs[3], concgs[3], concfs[1], concEdgeLabels[2], "4");
        await travelTo(3, 4);
        await calcScores1(4, concfs[4], concgs[4], concfs[3], concEdgeLabels[3], "5");
        await travelTo(3, 5);
        await calcScores1(5, concfs[5], concgs[5], concfs[3], concEdgeLabels[4], "6");
        await AnimationTime.WaitSeconds(1.0);

        await AnimationTime.WaitSeconds(3.0);
        concTexts[0].Active = false;
        concTexts[1].Active = false;
        concTexts[4].Active = false;
        concTexts[5].Active = false;

        concTexts[6].Active = true;
        foreach(var nd in ndnrs) {
            nd.Active = true;
        }
        
        await AnimationTransform.SmoothColor(concTexts[6], Color.WHITE, concTexts[6].Color, 0.5);
        await AnimationTime.WaitSeconds(1.0);
        concTexts[7].Active = true;
        await AnimationTransform.SmoothColor(concTexts[7], Color.WHITE, concTexts[7].Color, 0.5);

        await AnimationTime.WaitSeconds(1.0);

        concfbuf.Active = true;
        concgbuf.Active = true;
        concfbuf2.Active = true;
        concgbuf2.Active = true;
        concCurNode.Active = true;
        concCurNode.Text = "Current node: 1";

        _ = AnimationTransform.SmoothColor(concfbuf, Color.WHITE, concfbuf.Color, 0.5);
        _ = AnimationTransform.SmoothColor(concgbuf, Color.WHITE, concgbuf.Color, 0.5);
        _ = AnimationTransform.SmoothColor(concfbuf2, Color.WHITE, concfbuf2.Color, 0.5);
        await AnimationTransform.SmoothColor(concgbuf2, Color.WHITE, concgbuf2.Color, 0.5);
        //await AnimationTransform.SmoothColor(concCurNode, Color.WHITE, concCurNode.Color, 0.5);

        await AnimationTime.WaitSeconds(1.0);

        foreach(var concf in concfs)
            concf.Text = "∞";
        foreach(var concg in concgs)
            concg.Text = "∞";
        travelCircle.Color = concNodes[0].Color;

        await AnimationTime.WaitSeconds(3.0);

        var priQueueHandle = player.CreateHandle2D("priQueueHandle", new Vector2(1200.0f, 400.0f));
        var priQueue = new AnimatedTextList(priQueueHandle, 50.0f);

        // node 1 gScore
        Func<int, Text2D, Text2D, Text2D, Label, string, int, bool, Task> calcScores = async (nodeidx, gtext, ftext, prevNode, edgeLabel, nodename, addidx, replacepri) => {
            concCurNode.Text = "Current node: " + nodename;
            concfbuf.Text = "0.00";
            concgbuf.Text = "0.00";
            await AnimationTime.WaitSeconds(1.0);
            if(prevNode != null) {
                var prevT = world.Clone<Text2D>(prevNode);
                var val = prevT.Text;
                await world.AbsorbDestroy(prevT, 1.0f, concfbuf.state.position, true);
                concfbuf.Text = val;
            }
            if(edgeLabel != null) {
                var edgeL = world.Clone<Label>(edgeLabel);
                var val = edgeL.Text;
                await world.AbsorbDestroy(edgeL, 1.0f, concfbuf.state.position, true);
                concfbuf.Text = $"{float.Parse(concfbuf.Text)+float.Parse(val):0.00}";
            }
            // gscore -> fscore
            var gcop = world.Clone<Text2D>(concfbuf);
            gcop.Transform.Pos = gcop.state.position.ZOfst(-0.0018f);
            await world.AbsorbDestroy(gcop, 1.0f, concgbuf.state.position.ZOfst(-0.0005f), true);
            concgbuf.Text = concfbuf.Text;
            await AnimationTime.WaitSeconds(0.5);
            hLStart = concNodes[nodeidx].Transform.Pos.ZOfst(0.0005f);
            hLEnd = concNodes[5].Transform.Pos.ZOfst(0.0005f);
            hLine.Points = new Vector3[] {hLStart, hLEnd};
            hLine.Progression = 0.0f;
            hLabel.Text = "0.00";
            if(nodeidx == 3) {
                concEdgeLabels[4].Active = false;
            }
            await AnimationTransform.SmoothFloat((x) => {
                hLabel.Text = $"{(hLStart-hLEnd).Length*x:0.00}";
                hLine.Progression = x;
            }, 0.0f, 1.0f, 0.5f);
            await AnimationTime.WaitSeconds(0.5);
            var test1 = world.Clone<Label>(hLabel);
            if(nodeidx == 3) {
                concEdgeLabels[4].Active = true;
            }
            await world.AbsorbDestroy(test1, 1.0f, concgbuf.state.position+new Vector3(50.0f, 15.0f, -0.001f), true);
            concgbuf.Text = $"{float.Parse(concgbuf.Text)+float.Parse(hLabel.Text):0.00}";
            var tof = world.Clone<Text2D>(concfbuf);
            tof.Transform.Pos = tof.state.position.ZOfst(-0.0017f);
            await world.AbsorbDestroy(tof, 1.0f, gtext.state.position, true);
            gtext.Text = concfbuf.Text;
            tof = world.Clone<Text2D>(concgbuf);
            tof.Transform.Pos = tof.state.position.ZOfst(-0.0016f);
            await world.AbsorbDestroy(tof, 1.0f, ftext.state.position, true);
            ftext.Text = concgbuf.Text;
            hLabel.Text = "";
            hLine.Progression = 0.0f;
            if(!replacepri) {
                var tttt = world.Clone<Text2D>(ftext);
                tttt.Transform.Pos = tttt.state.position.ZOfst(-0.0019f);
                await priQueue.Add(tttt, addidx);
                priQueue.GetItem(addidx).Text += ", Node " + nodename;
            } else {
                var ite = priQueue.GetItem(addidx);
                var clon = world.Clone<Text2D>(ftext);
                var txt = clon.Text;
                await world.AbsorbDestroy(clon, 1.0f, ite.state.position.ZOfst(-0.002f), true);
                ite.Text = txt + ", Node " + nodename;
            }
        };

        int lastidx = -1;

        Func<int, Task> popPri = async (nodeidx) => {
            var item = priQueue.GetItem(0);
            _ = priQueue.RemoveItem(0);
            await world.AbsorbDestroy(item, 1.0f, concNodes[nodeidx].state.position);
            concNodes[nodeidx].Color = Color.ORANGE;
            if(lastidx != -1) {
                concNodes[lastidx].Color = concNodes[5].Color;
            }
            lastidx = nodeidx;
            //travelCircle.Active = false;
        };

        travelCircle.Active = true;
        travelCircle.Color = Color.RED;
        travelCircle.Transform.Pos = concNodes[0].Transform.Pos.ZOfst(-0.001f);
        await calcScores(0, concfs[0], concgs[0], null, null, "1", 0, false);
        await AnimationTime.WaitSeconds(1.0);
        await popPri(0);
        await AnimationTime.WaitSeconds(1.0);
        await travelTo(0, 1);
        await calcScores(1, concfs[1], concgs[1], concgs[0], concEdgeLabels[1], "3", 0, false);
        concArrow31.Active = true;
        await AnimationTime.WaitSeconds(1.0);
        await travelTo(0, 2);
        await calcScores(2, concfs[2], concgs[2], concfs[0], concEdgeLabels[0], "2", 1, false);
        concArrow21.Active = true;
        await AnimationTime.WaitSeconds(1.0);
        await popPri(1);
        await travelTo(1, 3);
        await calcScores(3, concfs[3], concgs[3], concfs[1], concEdgeLabels[2], "4", 1, false);
        concArrow43.Active = true;
        await AnimationTime.WaitSeconds(1.0);
        await popPri(2);
        await travelTo(2, 3);
        await calcScores(3, concfs[3], concgs[3], concfs[2], concEdgeLabels[5], "4", 0, true);
        //concArrow42.Active = true;
        var start112 = concArrow43.StartPoint;
        var end112 = concArrow43.EndPoint;
        await AnimationTransform.SmoothFloat((x) => {
            concArrow43.StartPoint = Vector3.Lerp(start112, concArrow42.StartPoint, x);
            concArrow43.EndPoint = Vector3.Lerp(end112, concArrow42.EndPoint, x);
        }, 0.0f, 1.0f, 0.5);
        await popPri(3);
        await AnimationTime.WaitSeconds(1.0);
        await travelTo(3, 4);
        await calcScores(4, concfs[4], concgs[4], concfs[3], concEdgeLabels[3], "5", 0, false);
        concArrow54.Active = true;
        await travelTo(3, 5);
        await calcScores(5, concfs[5], concgs[5], concfs[3], concEdgeLabels[4], "6", 0, false);
        await AnimationTime.WaitSeconds(1.0);
        await popPri(5);
        concNodes[5].Color = concNodes[0].Color;
        concNodes[3].Color = concNodes[0].Color;
        concArrow64.Active = true;
        travelCircle.Color = Color.GREEN;
        concTexts[3].Color = Color.GREEN;

        await AnimationTime.WaitSeconds(1.0);

        concArrow64.Color = Color.GREEN;
        await travelTo(5, 3);
        concArrow43.Color = Color.GREEN;
        await travelTo(3, 2);
        concArrow21.Color = Color.GREEN;
        await travelTo(2, 0);

        await AnimationTime.WaitSeconds(1.0);

        concEdgeLines[0].Color = Color.GREEN;
        concEdgeLines[5].Color = Color.GREEN;
        concEdgeLines[4].Color = Color.GREEN;

        await AnimationTime.WaitSeconds(3.0);

        concTexts[2].Active = false;
        concTexts[3].Active = false;

        foreach(var concf in concfs)
            concf.Active = false;
        foreach(var concg in concgs)
            concg.Active = false;
        concfbuf.Active = false;
        concgbuf.Active = false;
        concfbuf2.Active = false;
        concgbuf2.Active = false;
        concCurNode.Active = false;
        foreach(var nd in ndnrs) {
            nd.Active = false;
        }
        concTexts[6].Active = false;
        concTexts[7].Active = false;

        foreach(var text in priQueue.GetItems()) {
            world.Destroy(text);
        }

        await AnimationTransform.SmoothColor(titleText3, Color.WHITE, 0.5);
        world.Destroy(titleText3);


        // ASTAR EXAMPLES

        var gridTex = player.GetTextureResource("grid1");
        var gridTex2 = player.GetTextureResource("grid2");
        var grid = new Grid(20, 20);
        grid.CreateGrid(world, new Vector2(-5.0f, -5.0f), 0.5f, CIRCLE_RADIUS);
        grid.SetFromTexture(gridTex);

        await AnimationTransform.SmoothT<Vector3>((Vector3 pos) => {
            world.ActiveCamera.Transform.Pos = pos;
        }, world.ActiveCamera.Transform.Pos, startCamPos, 2.0);
        //world.ActiveCamera.Transform.Pos = startCamPos;

        var handle = player.CreateHandle2D("HW", new Vector2(0.0f, 50.0f), new Vector2(0.5f, 0.0f));
        var hw = world.Create2DText(handle, 22.0f, 0.0f, Color.BLACK, "A* on cell grid", new Vector2(0.5f, 0.0f), TextHorizontalAlignment.Center);

        var h2 = player.CreateHandle2D("Opened", new Vector2(30.0f, 0.0f), new Vector2(0.0f, 0.25f));
        int count = 0;
        var openTxt = world.Create2DText(h2, 14.0f, 0.0f, Color.BLACK, $"Opened: {count:D5}", new Vector2(0.0f, 0.25f), TextHorizontalAlignment.Right);
        _ = AnimationTransform.AnimT((double x) => {
            openTxt.Text = $"Edges visited: {opened}";
        }, 99999.0);
        /*var closeTxt = world.Create2DText(h2+new Vector2(0.0f, openTxt.Size*1.5f), 14.0f, 0.0f, Color.BLACK, $"Closed: {count:D5}", new Vector2(0.0f, 0.25f), TextHorizontalAlignment.Right);
        _ = AnimationTransform.AnimT((double x) => {
            closeTxt.Text = $"Closed: {closed:D5}";
        }, 99999.0);*/
        var lenTxt = world.Create2DText(h2+new Vector2(-200.0f, 200.0f), 20.0f, 0.0f, Color.BLACK, "", new Vector2(0.0f, 0.25f), TextHorizontalAlignment.Left);
        var lenTxt2 = world.Create2DText(h2+new Vector2(-200.0f, 230.0f), 20.0f, 0.0f, Color.BLACK, "", new Vector2(0.0f, 0.25f), TextHorizontalAlignment.Left);
        len = 0;
        _ = AnimationTransform.AnimT((double x) => {
            if(len > 1) {
                lenTxt.Text = $"Cost: {travelCost:0.00}";
                lenTxt2.Text = "Lowest possible is 23.49";
            } else {
                lenTxt.Text = "";
                lenTxt2.Text = "";
            }
        }, 9999999.0);

        var legendH = player.CreateHandle2D("Legend", new Vector2(100.0f, 30.0f), new Vector2(0.0f, 0.0f));
        var lRCirc = world.CreateCircle(15.0f, Color.YELLOW, true);
        var lRText = world.Create2DText(legendH + new Vector2(30.0f, -30.0f), openTxt.Size, 0.0f, Color.BLACK, "Current", Vector2.ZERO, TextHorizontalAlignment.Left, TextVerticalAlignment.Center);
        var lOCirc = world.CreateCircle(15.0f, Color.CYAN, true);
        var lOText = world.Create2DText(legendH + new Vector2(30.0f, 0.0f), openTxt.Size, 0.0f, Color.BLACK, "Open set", Vector2.ZERO, TextHorizontalAlignment.Left, TextVerticalAlignment.Center);
        var lCCirc = world.CreateCircle(15.0f, Color.GREEN, true);
        var lCText = world.Create2DText(legendH + new Vector2(30.0f, 30.0f), openTxt.Size, 0.0f, Color.BLACK, "Closed", Vector2.ZERO, TextHorizontalAlignment.Left, TextVerticalAlignment.Center);
        var lUCirc = world.CreateCircle(15.0f, new Color(100, 100, 100, 255), true);
        var lUText = world.Create2DText(legendH + new Vector2(30.0f, 60.0f), openTxt.Size, 0.0f, Color.BLACK, "Blocked", Vector2.ZERO, TextHorizontalAlignment.Left, TextVerticalAlignment.Center);
        var lSCirc = world.CreateCircle(15.0f, Color.ORANGE, true);
        var lSText = world.Create2DText(legendH + new Vector2(30.0f, 90.0f), openTxt.Size, 0.0f, Color.BLACK, "Start/End", Vector2.ZERO, TextHorizontalAlignment.Left, TextVerticalAlignment.Center);
        lOCirc.Transform.Pos = (Vector3)legendH;
        lCCirc.Transform.Pos = (Vector3)legendH + new Vector3(0.0f, 30.0f, 0.0f);
        lUCirc.Transform.Pos = (Vector3)legendH + new Vector3(0.0f, 60.0f, 0.0f);
        lSCirc.Transform.Pos = (Vector3)legendH + new Vector3(0.0f, 90.0f, 0.0f);
        lRCirc.Transform.Pos = (Vector3)legendH + new Vector3(0.0f, -30.0f, 0.0f);

        var scc = player.GetSceneEntityByName<Circle>("circle1");
        if(scc != null) {
            scc.Color = Color.CYAN;
        }

        /*var cam = new PerspectiveCamera();
        for(int i = 0; i <= 10; i++) {
            for(int j = 0; j < 10; j++) {
                float x = i*0.2f;
                float y = j*0.2f;
                var ray = cam.RayFromClip(new Vector2(-1.0f + x, -1.0f + y), 1.33f);
                for(int k = 0; k < 5; k++) {
                    var cir1 = world.CreateCircle(0.05f, Color.YELLOW);
                    cir1.Transform.Pos = ray.o + 0.2f*(float)k*ray.d;
                }
                //world.CreateSolidLine(new Vector2[] {ray.o, ray.o+ray.d}, 0.1f, Color.BROWN);
            }
        }*/

        //var arrow = world.CreateArrow(new Vector2(0.0f, 0.0f), new Vector2(1.0f, 1.0f), 0.1f, Color.RED, Color.BLACK);

        var h3 = player.CreateHandle2D("Right", new Vector2(-30.0f, 0.0f), new Vector2(1.0f, 0.25f));
        var heuTxt = world.Create2DText(h3, 14.0f, 0.0f, Color.BLACK, $"Heuristic: distance", new Vector2(1.0f, 0.25f), TextHorizontalAlignment.Left);

        var start = new CellId {x = 0, y = 5 };
        var end = new CellId {x = 15, y = 15 };

        Func<CellId, CellId, double> h1 = (from, to) => {
            var xdif = from.x-to.x;
            var ydif = from.y-to.y;
            return MathF.Sqrt(xdif*xdif + ydif*ydif);
        };

        Reset(grid, gridTex);
        grid.SetColor(start, Color.ORANGE);
        grid.SetColor(end, Color.ORANGE);
        await AnimationTime.WaitSeconds(1.0);
        await Astar(start, end, grid, hDist, h1);
        await AnimationTime.WaitSeconds(3.01);

        Reset(grid, gridTex);
        heuTxt.Text = $"Heuristic: 10*distance";

        await AnimationTime.WaitSeconds(1.0);

        grid.SetColor(start, Color.ORANGE);
        grid.SetColor(end, Color.ORANGE);
        await AnimationTime.WaitSeconds(1.0);
        await Astar(start, end, grid, hDist5, h1);
        await AnimationTime.WaitSeconds(3.0);

        Reset(grid, gridTex);
        heuTxt.Text = $"Heuristic: Manhattan distance";

        await AnimationTime.WaitSeconds(1.0);

        grid.SetColor(start, Color.ORANGE);
        grid.SetColor(end, Color.ORANGE);
        await AnimationTime.WaitSeconds(1.0);
        await Astar(start, end, grid, hManDist, h1);
        await AnimationTime.WaitSeconds(3.0);

        Reset(grid, gridTex);
        heuTxt.Text = $"Heuristic: 0";

        await AnimationTime.WaitSeconds(1.0);

        grid.SetColor(start, Color.ORANGE);
        grid.SetColor(end, Color.ORANGE);
        await AnimationTime.WaitSeconds(1.0);
        await Astar(start, end, grid, (CellId s, CellId e) => {
            return 0.0;
        }, h1);
        await AnimationTime.WaitSeconds(3.0);

        Reset(grid, gridTex2);
        heuTxt.Text = $"Heuristic: distance";
        
        await AnimationTime.WaitSeconds(1.0);

        grid.SetColor(start, Color.ORANGE);
        grid.SetColor(end, Color.ORANGE);
        await AnimationTime.WaitSeconds(1.0);
        await Astar(start, end, grid, hDist, h1);
        await AnimationTime.WaitSeconds(3.0);

        /*ClearArrows();
        heuTxt.Text = $"";
        hw.Text = "Dijkstra's algorithm on cell grid";
        grid.SetFromTexture(gridTex);

        await AnimationTime.WaitSeconds(1.0);

        grid.SetColor(start, Color.ORANGE);
        grid.SetColor(end, Color.ORANGE);
        await AnimationTime.WaitSeconds(1.0);
        await Dijkstra(start, end, grid, (CellId s, CellId e) => {
            return 1.0;
        });
        await AnimationTime.WaitSeconds(3.0);*/

        await AnimationTime.WaitSeconds(10);

    }
}
