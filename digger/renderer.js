var vertCode = `#version 300 es
    precision mediump float;
    layout(location = 0) in vec4 position;
    layout(location = 1) in vec4 color;
    uniform mat4 modelToClip;

    out vec4 v_color;

    void main(void) {
        gl_Position = modelToClip * position;
        v_color = color;
    }`;
var fragCode = `#version 300 es
    precision mediump float;
    out vec4 fragColor;
    in vec4 v_color;
    void main(void) {
        fragColor = v_color;
    }`;

var fragCodeSolid = `#version 300 es 
	precision mediump float;
	uniform vec4 _Color;
	out vec4 fragColor;
	void main(void) {
		fragColor = _Color;
	}`;

function Color(r, g, b, a) {
	this.r = r;
	this.g = g;
	this.b = b;
	this.a = a;
}

class Material {
	constructor(renderer, vertSrc, fragSrc, params)
	{
		if(!params) params = {};
		this.renderer = renderer;
		this.vertSrc = vertSrc;
		this.fragSrc = fragSrc;
		this.recompile();
		if(params.color) {
			this.color = params.color;
		} else {
			this.color = [1.0, 1.0, 1.0, 1.0];
		}
		this.v4Properties = [{name: "_Color", value: this.color}];
	}
	recompile() { // TODO: currently all materials have different program, we don't want that, we should reuse identical programs
		var gl = this.renderer.gl;
    	if(!this.vertShader)
			this.vertShader = gl.createShader(gl.VERTEX_SHADER);
	    gl.shaderSource(this.vertShader, this.vertSrc);
	    gl.compileShader(this.vertShader);
	    if(!gl.getShaderParameter(this.vertShader, gl.COMPILE_STATUS)) {
	        console.log('Vertex shader error:' + gl.getShaderInfoLog(this.vertShader));
	    }
	    if(!this.fragShader)
		    this.fragShader = gl.createShader(gl.FRAGMENT_SHADER);
	    gl.shaderSource(this.fragShader, this.fragSrc);
	    gl.compileShader(this.fragShader);
	    if(!gl.getShaderParameter(this.fragShader, gl.COMPILE_STATUS)) {
	        console.log('Fragment shader error: ' + gl.getShaderInfoLog(this.fragShader));
	    }
	    if(!this.program)
		    this.program = gl.createProgram();
	    gl.attachShader(this.program, this.vertShader);
	    gl.attachShader(this.program, this.fragShader);
	    gl.linkProgram(this.program);
    }

    bind() {
    	var gl = this.renderer.gl;
    	gl.useProgram(this.program);

	    this.v4Properties.forEach(el => {
	    	gl.uniform4fv(gl.getUniformLocation(this.program, el.name), el.value);
	    });
    }
}

class SolidMaterial extends Material{
	constructor(renderer, params) {
		super(renderer, vertCode, fragCodeSolid, params);
	}
}

class LineMaterial extends Material {
	constructor(renderer) {
		super(renderer, vertCode, fragCode);
	}
}

class Camera {
	constructor() {
		this.position = [0.0, 0.0, 0.0];
		this.width = 2.0;
		this.height = 2.0;
		this.depth = 2.0;
        this.renderscale = 1.0;
	}

	get worldToClipMatrix() { // aka projection*view
		var hw = this.width/2;
		var hh = this.height/2;
		var hd = this.depth/2;
		var l = this.position[0]-hw;
		var r = this.position[0]+hw;
		var t = this.position[1]+hh;
		var b = this.position[1]-hh;
		var f = this.position[2]+hd;
		var n = this.position[2]-hd;
        var s = this.renderscale;
        var scalem = [
            s, 0, 0, 0, 
            0, s, 0, 0, 
            0, 0, 1, 0, 
            0, 0, 0, 1
        ];
        var projm = [ 2/(r-l), 0, 0, 0, 0, 2/(t-b), 0, 0, 0, 0, -2/(f-n), 0, -(r+l)/(r-l), -(t+b)/(t-b), -(f+n)/(f-n), 1];
		return M4.mul(projm, scalem);
        //return projm;
	}
}


class Renderer {
	constructor(canvasId) {
		this.lineVertices = [];
		const canvas = document.getElementById(canvasId);
		var width = window.innerWidth;
		var height = window.innerHeight;
		canvas.width = width;
		canvas.height = height;
		this.gl = canvas.getContext("webgl2");
		this.gl.clientWidth = width;
		this.gl.clientHeight = height;
		this.gl.viewport(0, 0, width, height);
		this.camera = new Camera();
		this.camera.width = width;
		this.camera.height = height;
	    this.gl.enable(this.gl.DEPTH_TEST);
	    this.lineMaterial = new LineMaterial(this);
	}
	drawMesh(mesh, material, position = [0, 0], rotation = 0, scale = 1) {
		var gl = this.gl;
        if(!gl.isVertexArray(mesh.vao)) {
            throw "Must call Renderer.updateMesh() before drawMesh()";
        }
	    gl.bindVertexArray(mesh.vao);
	    material.bind();
		var mat = M4.trsAsArray(position, rotation, scale, scale);
		var proj = this.camera.worldToClipMatrix;
		var modelToClip = M4.mul(proj, mat);
		gl.uniformMatrix4fv(gl.getUniformLocation(material.program, "modelToClip"), false, modelToClip);
		gl.drawArrays(gl.LINES, 0, this.lineVertices.length/6);
	    gl.drawElements(gl.TRIANGLES, mesh.indices.length, gl.UNSIGNED_SHORT, 0);
	}
	drawLine(x1, y1, x2, y2, color) {
		this.lineVertices.push(x1, y1, color.r, color.g, color.b, color.a);
		this.lineVertices.push(x2, y2, color.r, color.g, color.b, color.a);
	}
	drawLineA(v1, v2, color) {
		this.lineVertices.push(v1[0], v1[1], color.r, color.g, color.b, color.a);
		this.lineVertices.push(v2[0], v2[1], color.r, color.g, color.b, color.a);
	}
	drawCircle(center, radius, segments, color) {
		var diam = radius*2;
		var step = diam / segments;
		var x0 = center[0];
		var y0 = center[1];
		for(var i = -segments; i < segments; i ++) {
			var x1 = i*step;
			var x2 = (i+1)*step;
			var dx1 = x1 - center[0]
			this.drawLineA([x0 + x1, y0 + Math.sqrt(radius*radius - x1*x1)], [x0 + x2, y0 + Math.sqrt(radius*radius - x2*x2)], color);
			this.drawLineA([x0 + x1, y0 - Math.sqrt(radius*radius - x1*x1)], [x0 + x2, y0 - Math.sqrt(radius*radius - x2*x2)], color);
		}
	}
	clear(color) {
		this.gl.clearColor(color.r, color.g, color.b, color.a);
		this.gl.clear(this.gl.COLOR_BUFFER_BIT);
	}
	endFrame() {
		var gl = this.gl;
		if(!this.lineVao) { // generate line vertex array object if not exist
			this.lineVao = gl.createVertexArray();
			this.lineVbo = gl.createBuffer();
			gl.bindVertexArray(gl.lineVao);
			gl.bindBuffer(gl.ARRAY_BUFFER, this.lineVbo);
			gl.enableVertexAttribArray(0);
			gl.vertexAttribPointer(0, 2, gl.FLOAT, false, 24, 0);
			gl.enableVertexAttribArray(1);
			gl.vertexAttribPointer(1, 4, gl.FLOAT, false, 24, 8);
		}
		if(this.lineVertices.length > 0) { // render all lines in 1 draw call
			gl.bindVertexArray(gl.lineVao);
			gl.bufferData(gl.ARRAY_BUFFER, new Float32Array(this.lineVertices), gl.DYNAMIC_DRAW);
			gl.useProgram(this.lineMaterial.program);
			var proj = this.camera.worldToClipMatrix;
			gl.uniformMatrix4fv(gl.getUniformLocation(this.lineMaterial.program, "modelToClip"), false, proj);
			gl.drawArrays(gl.LINES, 0, this.lineVertices.length/6);
		}
		gl.bindVertexArray(null);
		this.lineVertices.length = 0;
	}
	updateMesh(mesh) { // sync mesh data with opengl
		var gl = this.gl;
		if(mesh.vao == null)
	        mesh.vao = gl.createVertexArray();
	    if(mesh.vbo == null)
	        mesh.vbo = gl.createBuffer();
	    if(mesh.ebo == null)
	        mesh.ebo = gl.createBuffer();
	    gl.bindVertexArray(mesh.vao);
	    gl.bindBuffer(gl.ARRAY_BUFFER, mesh.vbo);
	    gl.bufferData(gl.ARRAY_BUFFER, new Float32Array(mesh.vertices), gl.STATIC_DRAW);
	    gl.bindBuffer(gl.ELEMENT_ARRAY_BUFFER, mesh.ebo);
	    gl.bufferData(gl.ELEMENT_ARRAY_BUFFER, new Uint16Array(mesh.indices), gl.STATIC_DRAW);
	    gl.enableVertexAttribArray(0);
	    gl.vertexAttribPointer(0, 3, gl.FLOAT, false, 0, 0);
	}
}

// column major 4x4 matrix
class M4 {
	static trsAsArray(t, r, sX, sY) { // 2D transform, rotate, scale matrix
		var rotQ = [Math.cos(r/2.0), 0.0, 0.0, Math.sin(r/2.0)];
		var ret = [1.0, 0.0, 0.0, 0.0,  0.0, 1.0, 0.0, 0.0,  0.0, 0.0, 1.0, 0.0,  0.0, 0.0, 0.0, 1.0];
	    ret[ 0] = (1.0-2.0*(rotQ[3]*rotQ[3]))*sX;
	    ret[ 1] = (rotQ[3]*rotQ[0])*sX*2.0;
	    ret[ 2] = 0.0;
	    ret[ 3] = 0.0;
	    ret[ 4] = (-rotQ[3]*rotQ[0])*sY*2.0;
	    ret[ 5] = (1.0-2.0*(rotQ[3]*rotQ[3]))*sY;
	    ret[ 6] = 0.0;
	    ret[ 7] = 0.0;
	    ret[ 8] = 0.0;
	    ret[ 9] = 0.0;
	    ret[10] = 1.0;
	    ret[11] = 0.0;
	    ret[12] = t[0];
	    ret[13] = t[1];
	    ret[14] = t[2];
	    ret[15] = 1.0;
	    return ret;
	}
	static mul(m1, m2) {
		console.assert(m1.length == 16 && m2.length == 16);
		var ret = [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0];
		for(var row = 0; row < 4; row++)
		for(var col = 0; col < 4; col++)
		for(var i = 0; i < 4; i++)
			ret[4*col+row] += m1[4*i + row]*m2[4*col + i];
		return ret;
	}
	static mulV4(m1, v1) {
		console.assert(m1.length == 16 && v1.length == 4);
		var ret = [0, 0, 0, 0];
		for(var row = 0; row < 4; row++)
		for(var i = 0; i < 4; i++)
			ret[row] += m1[4*i + row]*v1[i];
		return ret;
	}
}

// column major 2x2 matrix
class M2 {
	static mul(m1, m2) {
		console.assert(m1.length == 4 && m2.length == 4);
		var ret = [m1[0]*m2[0]+m1[2]*m2[1], 
			m1[1]*m2[0]+m1[3]*m2[1], 
			m1[0]*m2[2]+m1[2]*m2[3], 
			m1[1]*m2[2]+m1[3]*m2[3]];
		return ret;
	}
	static mulV2(m1, v1) {
		return [m1[0]*v1[0]+m1[2]*v1[1], m1[1]*v1[0]+m1[3]*v1[1]];
	}
	static mulScalar(m1, s) {
		assert(m1.length == 4);
		return [m1[0]*s, m1[1]*s, m1[2]*s, m1[3]*s];
	}
	static transpose(m) {
		return [m[0],m[2],m[1],m[3]];
	}
	static calculateEigenvalues(arr) {
		var a = arr[0], b = arr[2], c = arr[1], d = arr[3];
		var det = ((a+d)*(a+d)-4*(a*d-b*c));
		if(det < 0)
			return [NaN];
		if(det == 0)
			return [(a+d)/2];
		else {
			var sdet = Math.sqrt(det);
			return [(a+d+sdet)/2, (a+d-sdet)/2];
		}
	}
	static calculateEigenvectors(arr, evals) {
		var a = arr[0], b = arr[2], c = arr[1], d = arr[3];
		if(c != 0)
			return [V2.normalize([evals[0]-d, c]),V2.normalize([evals[1]-d, c])];
		if(b != 0)
			return [V2.normalize([b, evals[0]-a]), V2.normalize([b, evals[1]-a])];
		else
			return [[1, 0], [0, 1]];
	}
    static inverse(a) {
        // inverse determinant
        var d = 1 / (a[0]*a[3]-a[2]*a[1]);
        return [a[3]*d, -a[1]*d, -a[2]*d, a[0]*d];
    }
}

// 2 component vector
class V2 {
	static normalize(v) {
		var len = Math.sqrt(v[0]*v[0]+v[1]*v[1]);
		v[0] /= len;
		v[1] /= len;
		return v;
	}
	static mulM2(v, m) {
		return [v[0]*m[0]+v[1]*m[1], v[0]*m[2]+v[1]*m[3]];
	}
	static added(v1, v2) {
		return [v1[0]+v2[0], v1[1]+v2[1]];
	}
	static scaled(v, scale) {
		return [v[0]*scale, v[1]*scale];
	} 
	static dot(v1, v2) {
		return v1[0]*v2[0]+v1[1]*v2[1];
	}
}
