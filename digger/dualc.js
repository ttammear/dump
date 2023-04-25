class QEFSolver2D { 
    constructor() {
        this.normals = [];
        this.points = [];
    }
    addIntersection(point, normal) {
        this.points.push(point);
        this.normals.push(normal);
    }
    clear() {
        this.points.length = 0;
        this.normals.length = 0;
    }
    solve() {
        if(this.normals.length == 0)
            return null;
        if(this.points.length == 1) {
            return this.points[0];
        }
        console.assert(this.points.length == 2);
        var ns = this.normals;
        var ps = this.points;
        var A = [ns[0][0], ns[1][0], ns[0][1], ns[1][1]];
        var isRankDeficient = A[0]*A[3]-A[2]*A[1] == 0; // if determinant 0 then rank deficient
        if(isRankDeficient) // TODO: minimize plane equation closest to masspoint (this is good enough for most cases, especially if p[0] == p[1])
        {
            return ps[0];
        }
        var At = M2.transpose(A);
        var b = [V2.dot(ns[0], ps[0]), V2.dot(ns[1], ps[1])];
        var AtA = M2.mul(At, A);
        var invAtA = M2.inverse(AtA);
        var Atb = M2.mulV2(At, b);
        var x = M2.mulV2(invAtA, Atb);
        if(isNaN(x[0]) || isNaN(x[1]))
        {
            console.log("Result NaN!");
            return null;
        }
        return x;
    }
}

class DualContour2D {
    static binarySign(x) {
        if(x >= 0.0)
            return 1;
        else
            return -1;
    }
    static clampPointToSquare(point, squareMin, squareSize) {
        var dx = point[0] - squareMin[0];
        var dy = point[1] - squareMin[1];
        var ret = [point[0], point[1]];
        if(dx < 0)
            ret[0] = squareMin[0];
        else if(dx > squareSize)
            ret[0] = squareMin[0]+squareSize;
        if(dy < 0)
            ret[1] = squareMin[1];
        else if(dy > squareSize)
            ret[1] = squareMin[1]+squareSize;
        return ret; 
    }
    static clampPointToSquareTowardsMasspoint(point, mp, squareMin, squareMax) {
        if(DualContour2D.isInAABB(point, squareMin, squareMax)) {
            return point;
        }
        return DualContour2D.rayBoxIntersection(point, mp, squareMin, squareMax);
    }
    static isInAABB(point, minC, maxC) {
        return (point[0] >= minC[0] && point[0] <= maxC[0] && point[1] >= minC[1] && point[1] <= maxC[1]);
    }
    //   |   4   | 
    // -------------
    //   |       |
    //  2|       |3
    //   |       |
    // -------------
    //   |   1   | 
    // unoptimized version - just intersect all 4 line equations and check if intersection point is
    // inside the box
    static rayBoxIntersection(lineFrom, lineTo, boxMin, boxMax) {
        // TODO: I think this would be a lot cleaner if we somehow avoided 0 denominator
        // maybe by treating x as function of y for vertical ray?
        
        // ray line equation
        var intersections = [];
        var rdenom = (lineTo[0]-lineFrom[0]);

        var m = (lineTo[1]-lineFrom[1]) / rdenom;
        // y = mx+b -> x=(y+b)/m
        var b2 = lineFrom[1]-lineFrom[0]*m;
        // case 1 mx-b=ymin
        var iy, ix;
        if(m == 0) {
            // both lines horizontal
            if(lineFrom[1] == ymin) {
                // give point closest to lineTo on ymin edge
            }
            //intersections.push([null, null]);
        } else {
            iy = boxMin[1];
            ix = rdenom !== 0 ?(boxMin[1]-b2)/m : lineFrom[0];
            intersections.push([ix, iy]);
        }
        // case 2 (y+b)/m=xmin
        if(rdenom !== 0) {
            iy = m*boxMin[0]+b2;
            ix = boxMin[0];
            intersections.push([ix, iy]);
        } else {
            // both lines are vertical theoretically no intersection (or always intersecting if x is same)
            // TODO: this is problem when x is outside of box for vertical box edge intersection!
        }
        // case 3 same as 2, but ix = xmax
        if(rdenom !== 0) {
            iy = m*boxMax[0]+b2;
            ix = boxMax[0]; 
            intersections.push([ix, iy]);
        } else {
            // both lines are vertical theoretically no intersection (or always intersecting if x is same)
            // TODO: this is problem when x is outside of box for vertical box edge intersection!
        }
        // case 4 same as 1, but iy = ymax
        if(m == 0) {
            // both lines horizontal
            if(lineFrom[1] == ymax) {
                // give point closest to lineTo on ymax edge
            }
            //intersections.push([null, null]);
        } else {
            iy = boxMax[1];
            ix = rdenom !== 0 ? (boxMax[1]-b2)/m : lineFrom[0];
            intersections.push([ix, iy]);
        }

        var intsInBox = [];
        for(var i = 0; i < intersections.length; i++) {
           var generousMin = [boxMin[0]-0.001, boxMin[1]-0.001];
           var generousMax = [boxMax[0]+0.001, boxMax[1]+0.001];
           if(DualContour2D.isInAABB(intersections[i], generousMin, generousMax)) {
               intsInBox.push(intersections[i]);
           }
        }
        // find closest intersection by linear search
        // TODO: avoid this by determining where ray is coming from?
        var min = Number.MAX_VALUE;
        var minVal = null;
        for(var i = 0; i < intsInBox.length; i++) {
            var xdif = (lineFrom[0]-intsInBox[i][0]);
            var ydif = (lineFrom[1]-intsInBox[i][1]);
            var dist = xdif*xdif + ydif*ydif;
            if(dist < min) {
                minVal = intsInBox[i];
                min = dist;
            }
        }
        if(minVal == null) {
            throw "intersection is null?";
        }    
        return minVal;
    }

    static generateVertices(cells, fieldValueF, debugObj) {
        var qef = new QEFSolver2D();
        for(var i = 0; i < 100; i++) {
            for(var j = 0; j < 100; j++) {
                var massPoint = [0.0, 0.0];
                var points = 0;
                var intersection = false;
                var inside = false;
                var intCount = 0;
                for(var edge = 0; edge < 4; edge++) {
                    var edgeV = DualContour2D.cellEdges[edge];
                    var v0 = fieldValueF(i+edgeV[0], j+edgeV[1]);
                    var v1 = fieldValueF(i+edgeV[2], j+edgeV[3]);
                    if(v0 < 0) {
                        inside = true;
                    }
                    if(DualContour2D.binarySign(v0) != DualContour2D.binarySign(v1)) { // sign change on edge?
                        if(edge < 2 && i+1 < 100 && j+1 < 100) {
                            //intersections.push([i, j, edge&1]);
                        }
                        intersection = true;

                        // find intersection point along edge
                        // TODO: same point might be on 2 edges, if the intersection is exatly in corner, fix that?
                        const iterations = 1000;
                        const step = 1.0 / iterations;
                        for(var ti = 1; ti <= iterations; ti++) {
                            var t = ti*step;
                            if(ti == iterations) { // float accuracy issues...
                                t = 1.0;
                            }
                            var x = i+edgeV[0]+ (edgeV[2]-edgeV[0])*t;
                            var y = j+edgeV[1]+ (edgeV[3]-edgeV[1])*t;
                            var v1t = fieldValue(x, y);
                            if(DualContour2D.binarySign(v0) != DualContour2D.binarySign(v1t)) {
                                var normal = V2.scaled(fieldNormal(x, y), scale*10);
                                var point = [x*scale, y*scale];
                                massPoint[0] += point[0];
                                massPoint[1] += point[1];
                                debugObj.normals.push(normal);
                                debugObj.intersectionPoints.push(point);
                                intCount++;
                                qef.addIntersection(point, normal);
                                break;
                            }
                        }
                        points++;
                    }
                }
                if(intersection) {
                    massPoint[0] /= points;
                    massPoint[1] /= points;
                    debugObj.massPoints.push(massPoint);
                }
                
                var cellVertex = qef.solve();
                // clamp vertices inside their square
                if(cellVertex != null)
                {
                    var old = cellVertex;
                    cellVertex = DualContour2D.clampPointToSquareTowardsMasspoint(cellVertex, massPoint, [i*scale, j*scale], [i*scale+scale, j*scale+scale]);
                    debugObj.clampLines.push([i*scale+0.5*scale, j*scale+0.5*scale]);
                    debugObj.clampLines.push(cellVertex);
                    debugObj.clampLines.push(old);
                    cells[i][j].vertex = cellVertex;
                    debugObj.cellVertices.push(cellVertex);
                } 
                // if no intersection, just use middle point????
                else if(inside) {
                    cells[i][j].vertex = [(i+0.5)*scale, (j+0.5)*scale];
                }
                qef.clear();
            }
        }
    }

    static generateIsolineFromGrid(renderer, cells, width, height, fieldValueF) {
        var ret = [];
        for(var i = 0; i < width; i++) {
        for(var j = 0; j < height; j++) {
            for(var edge = 0; edge < 2; edge++) {
                var edgeV = DualContour2D.cellEdges[edge];
                var v0 = fieldValueF(i+edgeV[0], j+edgeV[1]);
                var v1 = fieldValueF(i+edgeV[2], j+edgeV[3]);
                // TODO: is this worth caching maybe?
                if(DualContour2D.binarySign(v0) != DualContour2D.binarySign(v1)) {
                    var c1 = cells[i][j].vertex;
                    if(edge == 1) { // vertical
                       var c2 = cells[i][j+1].vertex;
                       ret.push(c1[0], c1[1], c2[0], c2[1]);
                    } else { // horizontal
                       var c2 = cells[i+1][j].vertex;
                       ret.push(c1[0], c1[1], c2[0], c2[1]);
                    }
                }
            }
        }}
        return ret;
    }

    // NOTE: this is far from optimal, when using quad tree this could be fine though?
    // (generates vertex and triangles at every inner cell and doesn't reuse vertices or indices or indices)
    static generateFilledMeshFromGrid(renderer, cells, width, height, fieldValueF) {
        var mesh = {};
        mesh.vertices = [];
        mesh.indices = [];
        var midx = 0;
        const co = [[0,0],[1,0],[1,1],[0,1]];
        for(var i = 0; i < width; i++) {
        for(var j = 0; j < height; j++) {
            var isInside = fieldValueF(i+1, j+1) <= 0;
            // walk cells CCW and generator CCW triangles
            for(var idx = 0; idx < 4 && isInside; idx++) {
                var cell1 = cells[i+co[idx][0]][j+co[idx][1]];
                var c2i = (idx+1)&0x3;
                var cell2 = cells[i+co[c2i][0]][j+co[c2i][1]];
                if(typeof(cell1.vertex) !== 'undefined' && typeof(cell2.vertex) !== 'undefined') {
                    console.log("make triangle");
                    mesh.vertices.push(cell1.vertex[0], cell1.vertex[1], 0.0);
                    mesh.vertices.push(cell2.vertex[0], cell2.vertex[1], 0.0);
                    mesh.vertices.push((i+1)*scale, (j+1)*scale, 0.0);
                    mesh.indices.push(midx, midx+1, midx+2);
                    midx += 3;
                }
            }
        }}
        renderer.updateMesh(mesh);
        return mesh
    }

}
DualContour2D.cellEdges = [[1, 0, 1, 1], [0, 1, 1, 1], [0, 0, 0, 1], [0, 0, 1, 0]];


