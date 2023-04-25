var globalTime = 0.0;

var renderer = new Renderer("webgl-canvas");

var rectangleMaterial = new SolidMaterial(renderer, {color: [1.0, 0.0, 0.0, 1.0]});
var rectangleMaterial2 = new SolidMaterial(renderer, {color: [1.0, 1.0, 0.0, 1.0]});


// quad mesh
var mesh = {};
mesh.vertices = [
    -0.5, 0.5, 0.0,
    -0.5,-0.5, 0.0,
     0.5,-0.5, 0.0,
     0.5, 0.5, 0.0,
    ];
mesh.indices = [0, 1, 2, 0, 3, 2];
renderer.updateMesh(mesh);

renderer.camera.position = [0, 100, 0];

function length(p) {
    return Math.sqrt(p[0]*p[0] + p[1]*p[1]);
}

function circlesd(p, r) {
    return length(p) - r;
}

function boxsd(p, b) {
    var bd = [Math.abs(p[0]) - b[0], Math.abs(p[1]) - b[1]];
    var bdist = length([Math.max(bd[0], 0), Math.max(bd[1], 0)]) + Math.min(Math.max(bd[0], bd[1]), 0);
    return bdist;
}

function fieldValue(x, y) {
    var circlePos = [7.05, 8.0];
    var boxPos = [4.1, 8.1];
    var boxSize = [2.0, 8.0];
	var radius = 6.0; 
    var cdist = circlesd([x-circlePos[0], y-circlePos[1]], radius/2);
    var cdist2 = circlesd([x-15.0, y-10.0], radius);
    var bdist = boxsd([x-boxPos[0],y-boxPos[1]], boxSize);
    return Math.min(bdist,cdist,cdist2);
    //return cdist;
    //return bdist;
}

function fieldNormal(x, y) {
    var dx = 0.1;
    var dy = 0.1;
    var vdx = fieldValue(x+dx, y);
    var vdy = fieldValue(x, y+dy);
    return V2.normalize([vdx/dx, vdy/dy]);
}

// scale of the dualc grid
var scale = 10.0;
var renderscale = 1.0;

var mouseDown = false;
document.addEventListener('mousedown', e => {
    mouseDown = true;
});
document.addEventListener('mouseup', e => {
    mouseDown = false;
});
document.addEventListener('wheel', e => {
    var r = renderer.camera.renderscale;
    renderer.camera.renderscale -= r*0.01*e.deltaY;
    r = renderer.camera.renderscale;
    renderer.camera.renderscale = Math.max(0.0000001, r);
});

document.addEventListener('mousemove', e => {
    if(mouseDown === true) {
        renderer.camera.position[0] -= e.movementX;
        renderer.camera.position[1] += e.movementY;
    }
});

var debugMesh;

function animCallback(digger) {
    var debugObj = digger.debugObj;

	renderer.clear({r: 0, g: 0, b: 0, a: 1});

	globalTime += 0.01;
    renderer.drawMesh(mesh, rectangleMaterial, [-200.0, 0.0, -0.5], 0.0, 100.0);
    renderer.drawMesh(mesh, rectangleMaterial2, [400.0, 0.0, 0.0], globalTime, 100.0);

    // draw sine
    for(var i = -50; i < 50; i++) {
        var sinescale = 0.2;
        var xScale = 8.0;
        var yScale = 100.0;
        renderer.drawLine(i*xScale, Math.sin(globalTime+i*sinescale)*yScale, (i+1)*xScale, Math.sin(globalTime+(i+1)*sinescale)*yScale, new Color(0, 1, 0, 1));
    }

    const drawDebug = false;

    if(drawDebug) {
        //draw normals
        for(var i = 0; i < debugObj.intersectionPoints.length; i++) {
            renderer.drawCircle(debugObj.intersectionPoints[i], 1.0, 10, new Color(1, 0, 1, 1));
            renderer.drawLineA(debugObj.intersectionPoints[i], V2.added(debugObj.intersectionPoints[i], debugObj.normals[i]), new Color(0, 0, 1, 1));
        }
        for(var i = 0; i < debugObj.massPoints.length; i++) {
            renderer.drawCircle(debugObj.massPoints[i], 2.0, 10, new Color(1, 1, 1.0, 1));
        }
        for(var i = 0; i < debugObj.cellVertices.length; i++) {
            renderer.drawCircle(debugObj.cellVertices[i], 3.0, 10, new Color(0, 1, 1, 1));
            //renderer.drawLineA(debugObj.clampLines[i*3], debugObj.cellVertices[i], new Color(0.5, 0.5, 0.5, 1));
        }
        for(var i = 0; i < debugObj.clampLines.length; i+=3) {
            // 0 - cell center, 1 - after clamp, 2 - before clamp
            // from center to clamped
            renderer.drawLineA(debugObj.clampLines[i], debugObj.clampLines[i+1], new Color(1, 1, 1, 1));
            // from original to clmaped
            renderer.drawLineA(debugObj.clampLines[i+1], debugObj.clampLines[i+2], new Color(0, 1, 0, 1));
        }
        // draw grid
        var yc = new Color(1.0, 1.0, 0.0, 1.0);
        var maxl = 1200*(scale/60);
        // vertical line
        for(var i = 0; i < 50; i++) {
            renderer.drawLineA([0.0, i*scale], [maxl, i*scale], yc);
            renderer.drawLineA([0.0, i*scale], [0.0, maxl+i*scale], yc);
        }
        for(var i = 0; i < 50; i++) {
            renderer.drawLineA([i*scale, 0], [maxl+i*scale, 0], yc);
            renderer.drawLineA([i*scale, 0], [i*scale, maxl], yc);
        }
    }
    
    var lines = DualContour2D.generateIsolineFromGrid(renderer, digger.cells, 100, 100, fieldValue);
    for(var i = 0; i < lines.length; i+=4) {
        renderer.drawLine(lines[i], lines[i+1], lines[i+2], lines[i+3], new Color(0,0,1,1));
    }
    renderer.drawMesh(debugMesh, rectangleMaterial, [0.0, 0.0, 0.0], 0.0, 1.0);
    renderer.endFrame();
	requestAnimationFrame(digger.animframe);
}

function generateScene(digger) {
    var debugObj = digger.debugObj;
    debugObj.normals = [];
    debugObj.intersectionPoints = [];
    debugObj.clampLines = [];
    debugObj.cellVertices = [];
    debugObj.massPoints = [];
    DualContour2D.generateVertices(digger.cells, fieldValue, debugObj);
    var mesh = DualContour2D.generateFilledMeshFromGrid(renderer, digger.cells, 100, 100, fieldValue);
    debugMesh = mesh;
}

function runDigger() {
    var digger = {
        cells: [],
        debugObj: {},
    };
    for(var i = 0; i < 101; i++) {
    	digger.cells.push([]);
    	digger.cells[i].length = 101;
    	for(var j = 0; j < 101; j++) {
    		digger.cells[i][j] = {};
    	}
    }
    function animframe() {
        animCallback(digger);
    }
    digger.animframe = animframe;

    generateScene(digger);

    requestAnimationFrame(digger.animframe);
}

runDigger();

