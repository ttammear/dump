// Vertex attribute location bindings
//
#define POSITION_ATTRIBUTE_LOCATION     0
#define TEXCOORD_ATTRIBUTE_LOCATION     1
#define NORMAL_ATTRIBUTE_LOCATION       2
#define COLOR_ATTRIBUTE_LOCATION        3
#define TANGENT_ATTRIBUTE_LOCATION      4

// Uniform buffer bindings
//
#define UNIFORM_BUFFER_INSTANCEDATA    1
#define UNIFORM_BUFFER_MATRICES (1+MAX_INSTANCE_BUFFERS)
#define UNIFORM_BUFFER_OBJECTID (UNIFORM_BUFFER_MATRICES+MAX_MATRIX_BUFFERS)
// update my value and keep me last for clarity!
#define UNIFORM_BUFFER_COUNT (UNIFORM_BUFFER_OBJECTID+1)

static_assert(MAX_INSTANCE_BUFFERS >= INSTANCE_BUFFER_COUNT, "Instance buffer count can't be larger than MAX_INSTANCE_BUFFERS, consider increasing both.");
static_assert(UNIFORM_BUFFER_COUNT <= 36, "OpenGL does not guarantee more than 36 uniform buffer bindings, thus some hardware might not support it");

static const char *fallbackVert =
"#version 330 core\n"
"attribute vec4 coordinates;\n"
"layout (std140) uniform matrixBlock\n"
"{\n"
    "mat4 matrices[128];\n"
"};\n"
"void main(void) {\n"
    "gl_Position = matrices[gl_InstanceID] * coordinates;\n"
"}\n";
static const char *fallbackFrag = 
"#version 330 core\n"
"out vec4 outColor;\n"
"void main(void) {\n"
    "outColor = vec4(1.0, 0.0, 1.0, 1.0);\n"
"}\n";

static const char *vertShaderSrc =  "#version 330 core\n"
"attribute vec4 a_position;\n"
"attribute vec2 a_user0;\n"
"attribute vec4 a_user1;\n"
""
"layout (std140) uniform instanceBlock\n"
"{\n"
    "vec4 instanceColors[128];\n"
"};\n"
"layout (std140) uniform matrixBlock\n"
"{\n"
    "mat4 matrices[128];\n"
"};\n"
"layout (std140) uniform objectIdBlock\n"
"{\n"
    "int objectIds[128]; // waste of space sucks but shift and mask with ivec4 is too hard for old AMD drivers\n"
"};\n"
""
"varying vec2 v_texcoord;\n"
"varying vec4 v_color;\n"
"flat out int v_objectId;\n"
"uniform int instId;\n"
"void main(void) {\n"\
   "int id = gl_InstanceID;"
   "gl_Position = matrices[id] * a_position;\n"
   "v_texcoord = vec2(a_user0.x, 1.0 - a_user0.y);\n"
   "v_color = pow(a_user1 * instanceColors[id], vec4(2.2));\n"
   "v_objectId = objectIds[id];\n"
"}";

static const char *gizmoVertSrc = "#version 330 core\n"
"attribute vec4 a_position;\n"
"attribute vec2 a_user0;\n"
"attribute vec4 a_user1;\n"
""
"layout (std140) uniform instanceBlock\n"
"{\n"
    "vec4 instanceColors[128];\n"
"};\n"
"layout (std140) uniform matrixBlock\n"
"{\n"
    "mat4 matrices[128];\n"
"};\n"
"layout (std140) uniform objectIdBlock\n"
"{\n"
    "int objectIds[128]; // waste of space sucks but shift and mask with ivec4 is too hard for old AMD drivers\n"
"};\n"
""
"varying vec2 v_texcoord;\n"
"varying vec4 v_color;\n"
"flat out int v_objectId;\n"
"uniform int instId;\n"
"void main(void) {\n"\
   "int id = gl_InstanceID;"
   "float invScale = (matrices[id]*a_position).w;\n"
   "vec4 posCopy = a_position;"
   "posCopy.xyz *= invScale;\n"
   "gl_Position = matrices[id] * posCopy;\n"
   "v_texcoord = vec2(a_user0.x, 1.0 - a_user0.y);\n"
   "v_color = pow(a_user1 * instanceColors[id], vec4(2.2));\n"
   "v_objectId = objectIds[id];\n"
"}";

static const char *gizmoFragSrc = "#version 330 core\n"
"uniform sampler2D _MainTex;\n"
"varying vec2 v_texcoord;\n"
"varying vec4 v_color;\n"
"flat in int v_objectId;\n"
"out vec4 outColor;\n"
"layout(location = 1) out int outObjectId;\n"
"void main(void) {\n"
   "outColor = v_color;\n"
   "outObjectId = v_objectId;\n"
"}";

			
static const char *fragShaderSrc = "#version 330 core\n"
"uniform sampler2D _MainTex;\n"
"varying vec2 v_texcoord;\n"
"varying vec4 v_color;\n"
"flat in int v_objectId;\n"
"out vec4 outColor;\n"
"layout(location = 1) out int outObjectId;\n"
"void main(void) {\n"
   "//gl_FragColor = vec4(1.0, 0.0, 0.0, 1.0);\n"
   "outColor = texture2D(_MainTex, v_texcoord) * v_color;\n"
   "outObjectId = v_objectId;\n"
   "//gl_FragColor = vec4(v_texcoord.x, v_texcoord.y, 0.0, 1.0);\n"
"}";

static const char *fragShaderCutoutSrc = "#version 330 core\n"
"uniform sampler2D _MainTex;\n"
"varying vec2 v_texcoord;\n"
"varying vec4 v_color;\n"
"flat in int v_objectId;\n"
"out vec4 outColor;\n"
"layout(location = 1) out int outObjectId;\n"
"void main(void) {\n"
   "//gl_FragColor = vec4(1.0, 0.0, 0.0, 1.0);\n"
   "outColor = texture2D(_MainTex, v_texcoord) * v_color;\n"
   "if(outColor.a < 0.1) {\n"
   "discard;\n"
   "}\n"
   "outObjectId = v_objectId;\n"
   "//gl_FragColor = vec4(v_texcoord.x, v_texcoord.y, 0.0, 1.0);\n"
"}";

static const char *fragShaderSolidSrc = "#version 330 core\n"
"varying vec4 v_color;\n"
"flat in int v_objectId;\n"
"out vec4 outColor;\n"
"layout(location = 1) out int outObjectId;\n"
"void main(void) {\n"
   "outColor = v_color;\n"
   "outObjectId = v_objectId;\n"
"}";

static const char *uiVertSrc = 
"#version 330 core\n"
"uniform mat4 toClip;\n"
"attribute vec2 a_position;\n"
"attribute vec2 a_user0;\n"
"attribute vec4 a_user1;\n"
"varying vec2 v_texcoord;\n"
"varying vec4 v_color;\n"
"void main(void) {\n"
    "gl_Position = toClip * vec4(a_position, 0.0f, 1.0f);\n"
    "v_texcoord = vec2(a_user0.x, a_user0.y);\n"
    "v_color = pow(a_user1, vec4(2.2));\n"
"}";

static const char *uiFragSrc =
"#version 330 core\n"
"varying vec2 v_texcoord;\n"
"varying vec4 v_color;\n"
"uniform sampler2D _MainTex;\n"
"out vec4 outColor;\n"
"layout(location = 1) out int outObjectId;\n"
"void main(void) {\n"
    "outColor = v_color * texture2D(_MainTex, v_texcoord);\n"
    "outObjectId = 0xFFFFFFFF;\n"
"}";

static uint32_t glDefaultMesh[] = 
{
    0xbf000000, 0xbf000000, 0x3f000000, 0x3f800000, 0x00000000, 0x00000000, 0x00000000, 0xffffffff, 
    0x3f000000, 0xbf000000, 0x3f000000, 0x3f800000, 0x3f800000, 0x00000000, 0x00000000, 0xffffffff, 
    0x3f000000, 0x3f000000, 0x3f000000, 0x3f800000, 0x3f800000, 0x3f800000, 0x00000000, 0xffffffff, 
    0xbf000000, 0x3f000000, 0x3f000000, 0x3f800000, 0x00000000, 0x3f800000, 0x00000000, 0xffffffff, 
    0x3f000000, 0xbf000000, 0x3f000000, 0x3f800000, 0x00000000, 0x00000000, 0x00000000, 0xffffffff, 
    0x3f000000, 0xbf000000, 0xbf000000, 0x3f800000, 0x3f800000, 0x00000000, 0x00000000, 0xffffffff, 
    0x3f000000, 0x3f000000, 0xbf000000, 0x3f800000, 0x3f800000, 0x3f800000, 0x00000000, 0xffffffff, 
    0x3f000000, 0x3f000000, 0x3f000000, 0x3f800000, 0x00000000, 0x3f800000, 0x00000000, 0xffffffff, 
    0x3f000000, 0xbf000000, 0xbf000000, 0x3f800000, 0x00000000, 0x00000000, 0x00000000, 0xffffffff, 
    0xbf000000, 0xbf000000, 0xbf000000, 0x3f800000, 0x3f800000, 0x00000000, 0x00000000, 0xffffffff, 
    0xbf000000, 0x3f000000, 0xbf000000, 0x3f800000, 0x3f800000, 0x3f800000, 0x00000000, 0xffffffff, 
    0x3f000000, 0x3f000000, 0xbf000000, 0x3f800000, 0x00000000, 0x3f800000, 0x00000000, 0xffffffff, 
    0xbf000000, 0xbf000000, 0xbf000000, 0x3f800000, 0x00000000, 0x00000000, 0x00000000, 0xffffffff, 
    0xbf000000, 0xbf000000, 0x3f000000, 0x3f800000, 0x3f800000, 0x00000000, 0x00000000, 0xffffffff, 
    0xbf000000, 0x3f000000, 0x3f000000, 0x3f800000, 0x3f800000, 0x3f800000, 0x00000000, 0xffffffff, 
    0xbf000000, 0x3f000000, 0xbf000000, 0x3f800000, 0x00000000, 0x3f800000, 0x00000000, 0xffffffff, 
    0xbf000000, 0x3f000000, 0x3f000000, 0x3f800000, 0x00000000, 0x00000000, 0x00000000, 0xffffffff, 
    0x3f000000, 0x3f000000, 0x3f000000, 0x3f800000, 0x3f800000, 0x00000000, 0x00000000, 0xffffffff, 
    0x3f000000, 0x3f000000, 0xbf000000, 0x3f800000, 0x3f800000, 0x3f800000, 0x00000000, 0xffffffff, 
    0xbf000000, 0x3f000000, 0xbf000000, 0x3f800000, 0x00000000, 0x3f800000, 0x00000000, 0xffffffff, 
    0xbf000000, 0xbf000000, 0xbf000000, 0x3f800000, 0x00000000, 0x00000000, 0x00000000, 0xffffffff, 
    0x3f000000, 0xbf000000, 0xbf000000, 0x3f800000, 0x3f800000, 0x00000000, 0x00000000, 0xffffffff, 
    0x3f000000, 0xbf000000, 0x3f000000, 0x3f800000, 0x3f800000, 0x3f800000, 0x00000000, 0xffffffff, 
    0xbf000000, 0xbf000000, 0x3f000000, 0x3f800000, 0x00000000, 0x3f800000, 0x00000000, 0xffffffff, 
};

static unsigned short glDefaultMeshIdx[] =
{
	2,  1,   0,  0,  3,  2,
	6,  5,   4,  4,  7,  6,
    10, 9,   8,  8, 11, 10,
	14, 13, 12, 12, 15, 14,
	18, 17, 16, 16, 19, 18,
	22, 21, 20, 20, 23, 22
};


static uint32_t frameId;
internal GLMesh *get_mesh_safe(OpenGLRenderer *renderer, uint32_t meshId);

internal const char *opengl_type_to_str(GLint type)
{
    // no worries i used vim macro
    switch(type)
    {
    case GL_FLOAT: 	return "float";
    case GL_FLOAT_VEC2: 	return "vec2";
    case GL_FLOAT_VEC3: 	return "vec3";
    case GL_FLOAT_VEC4: 	return "vec4";
    /*
    case GL_DOUBLE: 	return "double";
    case GL_DOUBLE_VEC2: 	return "dvec2";
    case GL_DOUBLE_VEC3: 	return "dvec3";
    case GL_DOUBLE_VEC4: 	return "dvec4";
    */
    case GL_INT: 	return "int";
    case GL_INT_VEC2: 	return "ivec2";
    case GL_INT_VEC3: 	return "ivec3";
    case GL_INT_VEC4: 	return "ivec4";
    case GL_UNSIGNED_INT: 	return "unsigned int";
    case GL_UNSIGNED_INT_VEC2: 	return "uvec2";
    case GL_UNSIGNED_INT_VEC3: 	return "uvec3";
    case GL_UNSIGNED_INT_VEC4: 	return "uvec4";
    case GL_BOOL: 	return "bool";
    case GL_BOOL_VEC2: 	return "bvec2";
    case GL_BOOL_VEC3: 	return "bvec3";
    case GL_BOOL_VEC4: 	return "bvec4";
    case GL_FLOAT_MAT2: 	return "mat2";
    case GL_FLOAT_MAT3: 	return "mat3";
    case GL_FLOAT_MAT4: 	return "mat4";
    case GL_FLOAT_MAT2x3: 	return "mat2x3";
    case GL_FLOAT_MAT2x4: 	return "mat2x4";
    case GL_FLOAT_MAT3x2: 	return "mat3x2";
    case GL_FLOAT_MAT3x4: 	return "mat3x4";
    case GL_FLOAT_MAT4x2: 	return "mat4x2";
    case GL_FLOAT_MAT4x3: 	return "mat4x3";
    /*
    case GL_DOUBLE_MAT2: 	return "dmat2";
    case GL_DOUBLE_MAT3: 	return "dmat3";
    case GL_DOUBLE_MAT4: 	return "dmat4";
    case GL_DOUBLE_MAT2x3: 	return "dmat2x3";
    case GL_DOUBLE_MAT2x4: 	return "dmat2x4";
    case GL_DOUBLE_MAT3x2: 	return "dmat3x2";
    case GL_DOUBLE_MAT3x4: 	return "dmat3x4";
    case GL_DOUBLE_MAT4x2: 	return "dmat4x2";
    case GL_DOUBLE_MAT4x3: 	return "dmat4x3";
    */
    /*case GL_SAMPLER_1D: 	return "sampler1D";
    case GL_SAMPLER_2D: 	return "sampler2D";
    case GL_SAMPLER_3D: 	return "sampler3D";
    case GL_SAMPLER_CUBE: 	return "samplerCube";
    case GL_SAMPLER_1D_SHADOW: 	return "sampler1DShadow";
    case GL_SAMPLER_2D_SHADOW: 	return "sampler2DShadow";
    case GL_SAMPLER_1D_ARRAY: 	return "sampler1DArray";
    case GL_SAMPLER_2D_ARRAY: 	return "sampler2DArray";
    case GL_SAMPLER_1D_ARRAY_SHADOW: 	return "sampler1DArrayShadow";
    case GL_SAMPLER_2D_ARRAY_SHADOW: 	return "sampler2DArrayShadow";
    //case GL_SAMPLER_2D_MULTISAMPLE: 	return "sampler2DMS";
    //case GL_SAMPLER_2D_MULTISAMPLE_ARRAY: 	return "sampler2DMSArray";

    case GL_SAMPLER_CUBE_SHADOW: 	return "samplerCubeShadow";
    case GL_SAMPLER_BUFFER: 	return "samplerBuffer";
    case GL_SAMPLER_2D_RECT: 	return "sampler2DRect";
    case GL_SAMPLER_2D_RECT_SHADOW: 	return "sampler2DRectShadow";
    case GL_INT_SAMPLER_1D: 	return "isampler1D";
    case GL_INT_SAMPLER_2D: 	return "isampler2D";
    case GL_INT_SAMPLER_3D: 	return "isampler3D";
    case GL_INT_SAMPLER_CUBE: 	return "isamplerCube";
    case GL_INT_SAMPLER_1D_ARRAY: 	return "isampler1DArray";
    case GL_INT_SAMPLER_2D_ARRAY: 	return "isampler2DArray";
    //case GL_INT_SAMPLER_2D_MULTISAMPLE: 	return "isampler2DMS";
    //case GL_INT_SAMPLER_2D_MULTISAMPLE_ARRAY: 	return "isampler2DMSArray";
    case GL_INT_SAMPLER_BUFFER: 	return "isamplerBuffer";
    case GL_INT_SAMPLER_2D_RECT: 	return "isampler2DRect";
    case GL_UNSIGNED_INT_SAMPLER_1D: 	return "usampler1D";
    case GL_UNSIGNED_INT_SAMPLER_2D: 	return "usampler2D";
    case GL_UNSIGNED_INT_SAMPLER_3D: 	return "usampler3D";
    case GL_UNSIGNED_INT_SAMPLER_CUBE: 	return "usamplerCube";
    case GL_UNSIGNED_INT_SAMPLER_1D_ARRAY: 	return "usampler2DArray";
    case GL_UNSIGNED_INT_SAMPLER_2D_ARRAY: 	return "usampler2DArray";
    case GL_UNSIGNED_INT_SAMPLER_2D_MULTISAMPLE: 	return "usampler2DMS";
    case GL_UNSIGNED_INT_SAMPLER_2D_MULTISAMPLE_ARRAY: 	return "usampler2DMSArray";
    case GL_UNSIGNED_INT_SAMPLER_BUFFER: 	return "usamplerBuffer";
    case GL_UNSIGNED_INT_SAMPLER_2D_RECT: 	return "usampler2DRect";
    */
    default: return "unknown";
    }
}

internal void opengl_notify_sync(OpenGLRenderer *renderer, GLsync *sync, OnSyncAction_t action, void* userData)
{
    printf("Notify sync at frame %d\n", frameId);
    assert(renderer->numFreeSyncPoints > 0);
    uint32_t freeIdx = renderer->syncPointFreeList[--renderer->numFreeSyncPoints];
    GLSyncPoint *syncp = &renderer->syncPoints[freeIdx];
    syncp->sync = *sync;
    syncp->onSync = action;
    syncp->userData = userData;
    syncp->active = true;
}

internal void opengl_trigger_sync(OpenGLRenderer *renderer, uint32_t syncId)
{
    printf("Trigger sync at frame %d\n", frameId);
    assert(renderer->numFreeSyncPoints < GL_RENDERER_MAX_SYNC_POINTS);
    GLSyncPoint *syncp = &renderer->syncPoints[syncId];
    syncp->onSync(renderer, syncp->userData);
    syncp->sync = NULL;
    syncp->onSync = NULL;
    syncp->userData = NULL;
    syncp->active = false;
    renderer->syncPointFreeList[renderer->numFreeSyncPoints++] = syncId;
}

internal void opengl_process_program(GLuint program, GLMaterial *material)
{
    GLint nUniforms;
    glGetProgramiv(program, GL_ACTIVE_UNIFORMS, &nUniforms);
    printf("\nProgram had %d uniforms: \n", nUniforms);
    GLuint indices[nUniforms];
    for(int i = 0; i < nUniforms; i++)
        indices[i] = i;
    GLint resultTypes[nUniforms];
    GLint resultBlocks[nUniforms];
    GLint resultOffsets[nUniforms];
    GLint resultStrides[nUniforms];
    GLint resultSizes[nUniforms];
    glGetActiveUniformsiv(program, nUniforms, indices, GL_UNIFORM_TYPE, resultTypes);
    glGetActiveUniformsiv(program, nUniforms, indices, GL_UNIFORM_BLOCK_INDEX, resultBlocks);
    glGetActiveUniformsiv(program, nUniforms, indices, GL_UNIFORM_OFFSET, resultOffsets);
    glGetActiveUniformsiv(program, nUniforms, indices, GL_UNIFORM_ARRAY_STRIDE, resultStrides);
    glGetActiveUniformsiv(program, nUniforms, indices, GL_UNIFORM_SIZE, resultSizes);
    for(int i = 0; i < nUniforms; i++)
    {
        // TODO: dont need :)
        GLsizei len;
        GLint size;
        GLenum type;
        GLchar buf[64];
        glGetActiveUniform(program, i, 64, &len, &size, &type, buf);
        //printf("%s %s blockId: %d Offset: %d Stride: %d Size: %d\n", buf, opengl_type_to_str(resultTypes[i]), resultBlocks[i], resultOffsets[i], resultStrides[i], resultSizes[i]);
    }
    printf("---- end ----\n\n");

    // find the size of istance data 
    GLuint instanceBlock = glGetUniformBlockIndex(program, "instanceData");
    if(instanceBlock != GL_INVALID_INDEX)
    {
        GLuint instanceDataIndices[nUniforms];
        u32 numIndicesFound = 0;
        u32 curOffset = 0;
        i32 prevType = -1;

        for(int i = 0; i < nUniforms; i++)
        {
            if(resultBlocks[i] == instanceBlock)
            {
                // TODO: support for arrays
                assert(resultStrides[i] == 0);
                i32 curType = resultTypes[i];
                switch(curType)
                {
                    case GL_FLOAT:
                    case GL_INT:
                    case GL_UNSIGNED_INT:
                        curOffset = ALIGN_UP(curOffset, 4);
                        curOffset += 4;
                        break;
                    case GL_FLOAT_VEC2:
                    case GL_INT_VEC2:
                    case GL_UNSIGNED_INT_VEC2:
                        curOffset = ALIGN_UP(curOffset, 8);
                        curOffset += 8;
                        break;
                    case GL_FLOAT_VEC3:
                    case GL_INT_VEC3:
                    case GL_UNSIGNED_INT_VEC3:
                        curOffset = ALIGN_UP(curOffset, 16);
                        curOffset += 12;
                        break;
                    case GL_FLOAT_VEC4:
                    case GL_INT_VEC4:
                    case GL_UNSIGNED_INT_VEC4:
                        curOffset = ALIGN_UP(curOffset, 16);
                        curOffset += 16;
                        break;
                    case GL_FLOAT_MAT4:
                        curOffset = ALIGN_UP(curOffset, 16);
                        curOffset += 4*16;
                        break;
                    default: 
                        // TODO: add support for other types!
                        // https://www.khronos.org/registry/OpenGL/specs/gl/glspec45.core.pdf#page=159
                        assert(false);
                }
                prevType = curType;

                instanceDataIndices[numIndicesFound++] = i;
            }
        }
        material->perInstanceDataSize = ALIGN_UP(curOffset, 16);
        printf("expected offset for next: %d\n", material->perInstanceDataSize);
    }
    else
    {
        printf("WARNING: instanceData uniform block not found in shader!\n");
        material->perInstanceDataSize = 0;
    }
}

static inline void opengl_change_instance_buffer(OpenGLRenderer *renderer)
{
    renderer->curInstanceBufferIdx = ++renderer->curInstanceBufferIdx%INSTANCE_BUFFER_COUNT;
    renderer->curMatrixBufferIdx = ++renderer->curMatrixBufferIdx%MATRIX_BUFFER_COUNT;
}

internal void opengl_flush_instances(OpenGLRenderer *renderer, GLMesh *mesh, GLMaterial *mat, u32 instanceCount, void *uniformData, u32 uniformDataSize, void *matData, void *oidData)
{
    PROF_BLOCK();
    u32 bufIdx = renderer->curInstanceBufferIdx;
    glBindBuffer(GL_UNIFORM_BUFFER, renderer->instanceDataBuffers[bufIdx]);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, uniformDataSize, uniformData);

    // prepare mesh, program and uniforms
    GLuint program = (GLuint)mat->glProgram;
    glUseProgram(program);
    GLuint blockIdx = glGetUniformBlockIndex(program, "instanceBlock");
    if(blockIdx != GL_INVALID_INDEX)
        glUniformBlockBinding(program, blockIdx, UNIFORM_BUFFER_INSTANCEDATA+bufIdx);

    bufIdx = renderer->curMatrixBufferIdx;
    glBindBuffer(GL_UNIFORM_BUFFER, renderer->matrixBuffers[bufIdx]);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, instanceCount*sizeof(Mat4), matData);
    blockIdx = glGetUniformBlockIndex(program, "matrixBlock");
    if(blockIdx != GL_INVALID_INDEX)
        glUniformBlockBinding(program, blockIdx, UNIFORM_BUFFER_MATRICES+bufIdx);

    if(renderer->renderObjectId)
    {
        glBindBuffer(GL_UNIFORM_BUFFER, renderer->objectIdInstanceBuf);
        glBufferSubData(GL_UNIFORM_BUFFER, 0, instanceCount*sizeof(int32_t)*4, oidData);
        blockIdx = glGetUniformBlockIndex(program, "objectIdBlock");
        if(blockIdx != GL_INVALID_INDEX)
            glUniformBlockBinding(program, blockIdx, UNIFORM_BUFFER_OBJECTID);
    }

    glBindVertexArray(mesh->glVao);

    glDrawElementsInstanced(GL_TRIANGLES, mesh->numIndices, GL_UNSIGNED_SHORT, 0, instanceCount);

    /*GLuint instIDU = glGetUniformLocation(program, "instId");
    for(u32 j = 0; j < instanceCount; j++)
    {
        glUniform1i(instIDU, j);
        glDrawElements(GL_TRIANGLES, mesh->numIndices, GL_UNSIGNED_SHORT, 0);
    }*/

    //opengl_change_instance_buffer(renderer);
}

internal void calculate_vertex_buffer_size(MeshQuery *mq, uint32_t *size, uint32_t *stride, uint32_t offsetArr[])
{
    uint32_t vertexCount = mq->vertexCount;
    uint32_t perVertexSize = sizeof(V4);
    for(int i = 0; i < MAX_ATTRIBUTE_BUFFERS; i++)
    {
        // TODO: handle invalid type
        offsetArr[i] = perVertexSize;
        perVertexSize += s_vertexAttributeTypeSizes[mq->attributeTypes[i]];
    }
    *size = vertexCount * perVertexSize;
    *stride = perVertexSize;
}

internal GLMesh* get_mesh(OpenGLRenderer *renderer, uint32_t meshId)
{
    if(meshId >= buf_len(renderer->meshes))
    {
        // TODO: log error
        return NULL;
    }
    GLMesh *ret = &renderer->meshes[meshId];
    assert(ret->id == meshId);
    return ret;
}

internal GLMesh *get_mesh_safe(OpenGLRenderer *renderer, uint32_t meshId)
{
    if(meshId >= buf_len(renderer->meshes))
    {
        return &renderer->meshes[0];
    }
    GLMesh *ret = &renderer->meshes[meshId];
    assert(ret->id == meshId);
    return ret;
}

internal void opengl_handle_mesh_query(OpenGLRenderer *renderer, MeshQuery *mq, void *userData)
{
    uint32_t meshId = mq->meshId;
    if(meshId == 0)
    {
        meshId = buf_len(renderer->meshes);
        GLMesh mesh = {.id = meshId};
        buf_push(renderer->meshes, mesh);
        assert(buf_len(renderer->meshes) < 1000); // TODO
        printf("new mesh %d\n", meshId);
    }

    uint32_t vertBufSize, vertexStride;
    uint32_t attribOffsets[MAX_ATTRIBUTE_BUFFERS];
    calculate_vertex_buffer_size(mq, &vertBufSize, &vertexStride, attribOffsets);
    uint32_t indexBufSize = mq->indexCount * sizeof(uint16_t);
    if(vertBufSize == 0 || indexBufSize == 0)
    {
        // TODO: log error
        return;
    }

    GLMesh *mesh = get_mesh(renderer, meshId);
    if(!mesh)
        return;
    if(mesh->state != GL_Mesh_State_Init && mesh->state != GL_Mesh_State_Ready)
    {
        // TODO: log error
        return;
    }

    GLuint vao = mesh->glVao;
    if(vao == 0)
    {
        glGenVertexArrays(1, &vao);
        mesh->glVao = (uintptr_t)vao;
    }
    glBindVertexArray(vao);

    GLuint vbo = (GLuint)mesh->glVbo;
    if(vbo == 0)
    {
        glGenBuffers(1, &vbo);
        mesh->glVbo = (uintptr_t)vbo;
    }
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    if(mesh->vertexBufferSize < vertBufSize)
    {
        glBufferData(GL_ARRAY_BUFFER, vertBufSize, 0, GL_STATIC_DRAW);
        mesh->vertexBufferSize = vertBufSize;
    }
    // TODO: if the mesh is being draw we might need sync (defer this command)
    void *vertPtr = glMapBufferRange(GL_ARRAY_BUFFER, 0, vertBufSize, GL_MAP_WRITE_BIT | GL_MAP_UNSYNCHRONIZED_BIT);
    printf("buf size %d\n", vertBufSize);

    GLuint ebo = (GLuint)mesh->glEbo;
    if(ebo == 0)
    {
        glGenBuffers(1, &ebo);
        mesh->glEbo = (uintptr_t)ebo;
    }
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    if(mesh->indexBufferSize < indexBufSize)
    {
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexBufSize, 0, GL_STATIC_DRAW);
        mesh->indexBufferSize = indexBufSize;
    }
    // TODO: if the mesh is being drawn we might need sync (defer this command)
    void *indexPtr = glMapBufferRange(GL_ELEMENT_ARRAY_BUFFER, 0, indexBufSize, GL_MAP_WRITE_BIT | GL_MAP_UNSYNCHRONIZED_BIT);

    mesh->vertexStride = vertexStride;
    mesh->numVertices = mq->vertexCount;
    mesh->numIndices = mq->indexCount;
    mesh->state = GL_Mesh_State_Dirty;
    memcpy(mesh->attribOffsets, &attribOffsets, sizeof(mesh->attribOffsets));
    memcpy(mesh->attribTypes, &mq->attributeTypes, sizeof(mesh->attribTypes));

    glBindVertexArray(0);

    RenderMessage msg = {};
    msg.type = Render_Message_Mesh_Query_Result;
    msg.usrData = userData;
    msg.meshQueryResult.meshId = meshId;
    msg.meshQueryResult.vertBufPtr = vertPtr;
    msg.meshQueryResult.idxBufPtr = indexPtr;
    msg.meshQueryResult.dataBufPtr = NULL; // TODO
    ring_queue_enqueue(RenderMessage, &renderer->renderer.ch.fromRenderer, &msg);
}

internal void opengl_mesh_ready(OpenGLRenderer *renderer, GLMesh* mesh)
{
    assert(mesh->state == GL_Mesh_State_Wait_Sync);
    mesh->state = GL_Mesh_State_Ready;
    glDeleteSync(mesh->fence);
    mesh->fence = NULL;
    RenderMessage msg = {};
    msg.type = Render_Message_Mesh_Ready;
    msg.meshR.meshId = mesh->id;
    msg.usrData = mesh->userData2;
    ring_queue_enqueue(RenderMessage, &renderer->renderer.ch.fromRenderer, &msg);
}

internal void opengl_handle_mesh_update(OpenGLRenderer *renderer, MeshUpdate *mu, void *userData)
{
    // TODO: handle errors
    uint32_t meshId = mu->meshId;
    GLMesh *mesh = get_mesh(renderer, meshId);
    if(!mesh)
        return;
    if(mesh->state != GL_Mesh_State_Dirty)
        return; // TODO log error

/*    if(mu->numSections > MAX_MESH_SECTIONS)
    {
        fprintf(stderr, "Mesh had more sections than allowed! (%d vs %d)\n", mu->numSections, MAX_MESH_SECTIONS);
        return;
    }

    for(int i = 0; i < mu->numSections; i++)
    {
        if(mu->sections[i].offset + mu->sections[i].count >= mesh->numIndices)
        {
            fprintf(stderr, "Mesh section out of range (%d-%d, but max %d)\n", mu->sections[i].offset, mu->sections[i].offset + mu->sections[i].count, mesh->numIndices);
            mesh->sections[i].offset = 0;
            mesh->sections[i].count = 0;
        }
        else
        {
           mesh->sections[i].offset = mu->sections[i].offset; 
           mesh->sections[i].count = mu->sections[i].count; 
        }
    }*/

    assert(mesh->glVbo != 0);
    assert(mesh->glEbo != 0);
    assert(mesh->glVao != 0);
    GLuint vao, vbo, ebo;
    vao = (GLuint)mesh->glVao;
    vbo = (GLuint)mesh->glVbo;
    ebo = (GLuint)mesh->glEbo;
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glUnmapBuffer(GL_ARRAY_BUFFER);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, false, mesh->vertexStride, 0);
    for(int i = 0; i < MAX_ATTRIBUTE_BUFFERS; i++)
    {
        uint32_t type = mesh->attribTypes[i];
        uint32_t numfloats = s_vertexAttributeTypePrims[type];
        if(type != Vertex_Attribute_Type_None)
        {
            glEnableVertexAttribArray(i+1);
            glVertexAttribPointer(i+1, numfloats, GL_FLOAT, false, mesh->vertexStride, (GLvoid*)(uintptr_t)mesh->attribOffsets[i]);
        }
        else
            glDisableVertexAttribArray(i+1);
    }
    mesh->state = GL_Mesh_State_Wait_Sync;
    mesh->fence = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
    mesh->userData2 = userData;
    opengl_notify_sync(renderer, &mesh->fence, (OnSyncAction_t)opengl_mesh_ready, mesh);
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

internal GLMaterial *get_material(OpenGLRenderer *renderer, uint32_t matId)
{
    if(matId >= buf_len(renderer->materials))
    {
        return &renderer->materials[0];
    }
    GLMaterial *ret = &renderer->materials[matId];
    assert(ret->id == matId);
    return ret;
}

internal void opengl_handle_material_query(OpenGLRenderer *renderer, MaterialQuery *mq, void *userData)
{
    uint32_t materialId = mq->materialId;
    if(materialId == 0)
    {
        materialId = buf_len(renderer->materials);
        GLMaterial mat = {.id = materialId};
        buf_push(renderer->materials, mat);
        assert(buf_len(renderer->materials) < 1000); //TODO
    }
    GLMaterial *material = get_material(renderer, materialId);
    if(material == NULL)
    {
        fprintf(stderr, "get_material returned NULL\n");
        return;
    }
    if(mq->shaderId >= Shader_Type_Count)
    {
        fprintf(stderr, "material shaderId out of range, using Shader_Type_None!\n");
        mq->shaderId = Shader_Type_None;
    }
    material->shaderId = mq->shaderId;
    material->glProgram = renderer->builtinPrograms[mq->shaderId];
    switch(mq->shaderId)
    {
        case Shader_Type_Unlit_Textured_Cutout:
        case Shader_Type_Unlit_Textured:
            material->perInstanceDataSize = sizeof(struct UnlitTexturedIData);
            break;
        case Shader_Type_Unlit_Color:
            material->perInstanceDataSize = sizeof(struct UnlitColorIData);
            break;
        default:
            material->perInstanceDataSize = 0;
            break;
    };
    material->userData2 = userData;
    material->iData = mq->iData; // @OPTIMIZE: dont have to copy full

    RenderMessage msg = {};
    msg.type = Render_Message_Material_Ready;
    msg.matR.materialId = material->id;
    msg.usrData = material->userData2;
    ring_queue_enqueue(RenderMessage, &renderer->renderer.ch.fromRenderer, &msg);
}

static inline GLTexture *get_texture(OpenGLRenderer *renderer, uint32_t texId)
{
    if(texId >= buf_len(renderer->textures))
    {
        // TODO: log error
        return NULL;
    }
    GLTexture *ret = &renderer->textures[texId];
    assert(ret->id == texId);
    return ret;
}

internal void opengl_handle_texture_query(OpenGLRenderer *renderer, TextureQuery *tq, void *userData)
{
    // TODO: max size?
    if(tq->width == 0 || tq->height == 0)
    {
        // TODO: log error
        return;
    }

    uint32_t textureId = tq->textureId;
    if(textureId == 0)
    {
        textureId = buf_len(renderer->textures);
        GLTexture tex = {.id = textureId};
        buf_push(renderer->textures, tex);
        assert(buf_len(renderer->textures)<1000);//TODO;
    }
    GLTexture *tex = get_texture(renderer, textureId);
    if(!tex)
        return;
    if(tex->state != GL_Texture_State_Init && tex->state != GL_Texture_State_Ready)
    {
        // TODO: log error
        return;
    }

    GLuint pbo = (GLuint)tex->glPub;
    if(pbo == 0)
    {
        glGenBuffers(1, &pbo);
        tex->glPub = (uintptr_t)pbo;
    }
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbo);

    assert(tq->format == Texture_Format_RGBA);
    uint32_t requiredSize = 4*tq->width*tq->height;
    if(tex->bufferSize < requiredSize)
    {
        glBufferData(GL_PIXEL_UNPACK_BUFFER, requiredSize, NULL, GL_STATIC_DRAW);
        tex->bufferSize = requiredSize;
    }
    // TODO: if the texture is being drawn then we might need sync
    void *dataPtr = glMapBufferRange(GL_PIXEL_UNPACK_BUFFER, 0, requiredSize, GL_MAP_WRITE_BIT | GL_MAP_UNSYNCHRONIZED_BIT);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);

    tex->width = tq->width;
    tex->height = tq->height;
    tex->format = tq->format;
    tex->filter = tq->filter;
    tex->state = GL_Texture_State_Dirty;
    tex->userData2 = userData;

    RenderMessage response = {};
    response.type = Render_Message_Texture_Query_Response;
    response.texQR.textureId = textureId;
    response.texQR.userData = tq->userData;
    response.usrData = userData;
    response.texQR.onComplete = tq->onComplete;
    response.texQR.textureDataPtr = dataPtr;
    ring_queue_enqueue(RenderMessage, &renderer->renderer.ch.fromRenderer, &response);
}

internal void opengl_texture_ready(OpenGLRenderer *renderer, GLTexture *tex)
{
    assert(tex->state == GL_Texture_State_Wait_Sync);
    tex->state = GL_Texture_State_Ready;
    glDeleteSync(tex->fence);
    tex->fence = NULL;
    RenderMessage msg = {};
    msg.type = Render_Message_Texture_Ready;
    msg.texR.textureId = tex->id;
    msg.texR.onComplete = tex->onComplete;
    msg.texR.userData = tex->userData;
    msg.usrData = tex->userData2;
    ring_queue_enqueue(RenderMessage, &renderer->renderer.ch.fromRenderer, &msg);
}

internal void opengl_handle_texture_update(OpenGLRenderer *renderer, TextureUpdate *tu, void *usrData)
{
    uint32_t textureId = tu->textureId;
    if(textureId == 0)
    {
        // TODO: log error
        fprintf(stderr, "NOT A TEXTURE\n");
        return;
    }
    GLTexture *tex = get_texture(renderer, textureId);
    if(!tex)
        return;
    if(tex->state != GL_Texture_State_Dirty)
    {
        // TODO: log error
        return;
    }
    GLuint pbo = (GLuint)tex->glPub;
    assert(pbo != 0);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbo);
    glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);

    GLuint gltex = (GLuint)tex->glTex;
    if(gltex == 0)
    {
        glGenTextures(1, &gltex);
        tex->glTex = (uintptr_t)gltex;
    }
    glBindTexture(GL_TEXTURE_2D, gltex);
    assert(tex->format = Texture_Format_RGBA); // TODO: more formats
    glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB8_ALPHA8, tex->width, tex->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
    switch(tex->filter)
    {
        case Texture_Filter_None:
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            break;
        case Texture_Filter_Bilinear:
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            break;
        case Texture_Filter_Trilinear:
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            glGenerateMipmap(GL_TEXTURE_2D);
            break;
        default:
            assert(false);
            break;
    }
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    tex->state = GL_Texture_State_Wait_Sync;
    tex->fence = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
    opengl_notify_sync(renderer, &tex->fence, (OnSyncAction_t)opengl_texture_ready, tex);

    tex->userData = tu->userData;
    tex->userData2 = usrData;
    tex->onComplete = tu->onComplete;

    //glBindTexture(GL_TEXTURE_2D, 0);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
}

internal void opengl_handle_object_id_samples(OpenGLRenderer *renderer, SampleObjectId *soid)
{
    PROF_BLOCK();
    RenderMessage msg = {};

    PROF_START_STR("Data to PBO");
    assert(renderer->objectIdPbo != 0); // buffer not initialized
    glBindBuffer(GL_PIXEL_PACK_BUFFER, renderer->objectIdPbo);
    glBindTexture(GL_TEXTURE_2D, renderer->objectIdFboTex);
#ifndef AIKE_GLES
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RED_INTEGER, GL_INT, 0); 
#else
#warning no glGetTexImage on GLES
#endif
    glBindTexture(GL_TEXTURE_2D, 0);
    PROF_END();
    if(!renderer->renderObjectId)
    {
        fprintf(stderr, "ObjectId samples requested, but feature not enabled!\n");
        goto respond;
    }
    // TODO: wait fence then get
    // TODO: hard coded constant
    PROF_START_STR("Data to CPU");
    for(int i = 0; i < soid->sampleCount; i++)
    {
        uint32_t w = renderer->fboWidth;
        uint32_t h = renderer->fboHeight;
        int32_t x = MIN(w-1, w*soid->normalizedSampleCoords[i].x);
        x = MAX(x, 0);
        int32_t y = MIN(h-1, h - h*soid->normalizedSampleCoords[i].y);
        y = MAX(y, 0);
        // TODO: maybe theres something more efficient for random access like this?
#ifndef AIKE_GLES
        glGetBufferSubData(GL_PIXEL_PACK_BUFFER, y*w*4+x*4, 4, soid->buffer);
#else
#warning no glGetBufferSubData on GLES
#endif
    }
    glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
    PROF_END();

respond:
    msg.type = Render_Message_Sample_Object_Ready;
    msg.sampleOR.onComplete = soid->onComplete;
    msg.sampleOR.userData = soid->userData;
    ring_queue_enqueue(RenderMessage, &renderer->renderer.ch.fromRenderer, &msg);
}

internal bool opengl_process_messages(OpenGLRenderer *renderer)
{
    PROF_BLOCK();
    Renderer *r = &renderer->renderer;
    RenderMessage msg;
    while(ring_queue_dequeue(RenderMessage, &r->ch.toRenderer, &msg))
    {
        switch(msg.type)
        {
            case Render_Message_Mesh_Query:
                printf("OpenGL received mesh query!\n");
                opengl_handle_mesh_query(renderer, &msg.meshQuery, msg.usrData);
                break;
            case Render_Message_Mesh_Update:
                printf("OpenGL received mesh update!\n");
                opengl_handle_mesh_update(renderer, &msg.meshUpdate, msg.usrData);
                break;
            case Render_Message_Material_Query:
                printf("OpenGL received material query!\n");
                opengl_handle_material_query(renderer, &msg.matQ, msg.usrData);
                break;
            case Render_Message_Texture_Query:
                printf("OpenGL recived texture query!\n");
                opengl_handle_texture_query(renderer, &msg.texQ, msg.usrData);
                break;
            case Render_Message_Texture_Update:
                printf("OpenGL received texture update!\n");
                opengl_handle_texture_update(renderer, &msg.texU, msg.usrData);
                break;
            case Render_Message_Sample_Object_Id:
                opengl_handle_object_id_samples(renderer, &msg.sampleO);
                break;
            case Render_Message_Stop:
                // TODO: CLEAN UP
                fprintf(stderr, "PLEASE CLEAN UP THE STATEE!!!\n");
                // detach glcontext from this thread
                renderer->renderer.platform->make_window_current(renderer->renderer.platform, NULL);
                return false;
            case Render_Message_Screen_Resize:
                renderer->windowWidth = msg.screenR.width;
                renderer->windowHeight = msg.screenR.height;
                //glViewport(0, 0, msg.screenR.width, msg.screenR.height);
                break;
            default:
                fprintf(stderr, "OpenGL renderer: unknown message type!\n");
                break;
        }
    }
    return true;
}

internal void opengl_render_view(OpenGLRenderer *renderer, RenderViewBuffer *rbuf)
{
    PROF_BLOCK();
    RenderView *view = &rbuf->view;
    if(view->space == NULL)
        return;
    u32 meshCount = view->space->numEntries;
    
    // TODO: temp allocators
    u8 idbuf[renderer->uniBufSize];
    u8 mbuf[renderer->uniBufSize];
    uint32_t oidBuf[renderer->uniBufSize/4];

    u32 idofst;
    u32 mofst;

    PROF_START_STR("Extract matrices");
    // extract matrices
    _Alignas(16) Mat4 matBuf[view->matrixCount*TT_SIMD_32_WIDTH];
    assert(((uintptr_t)&view->tmatrixBuf & 0xF) == 0);
    assert(((uintptr_t)matBuf & 0xF) == 0);

    for(int i = 0; i < view->matrixCount/TT_SIMD_32_WIDTH; i++)
    {
        mat4_extract_all_sse2(&matBuf[i*TT_SIMD_32_WIDTH], &view->tmatrixBuf[i]);
    }
    PROF_END();

    for(u32 i = 0; i < meshCount; i++)
    {
        idofst = 0;
        mofst = 0;
        RenderMeshEntry *mentry = view->space->meshEntries[i];
        GLMaterial *material = get_material(renderer, mentry->materialId);
        GLMesh *mesh = get_mesh_safe(renderer, mentry->meshId);
        GLTexture *tex;

        u32 instanceCount = mentry->numInstances;
        assert(instanceCount != 0);

        u32 instId = 0;
        u32 instRunId = 0;
        void *idptr = NULL;
        u32 idsize;
        switch(material->shaderId)
        {
            case Shader_Type_None:
                idsize = 0;
                glDisable(GL_BLEND);
                glEnable(GL_DEPTH_TEST);
                break;
            case Shader_Type_Unlit_Textured:
            case Shader_Type_Unlit_Textured_Cutout:
                idsize = 16;
                idptr = &material->iData.unlitTextured.color;
                tex = get_texture(renderer, material->iData.unlitTextured.textureId);
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, tex->glTex);
                glDisable(GL_BLEND);
                glEnable(GL_DEPTH_TEST);
                break;
            case Shader_Type_Unlit_Vertex_Color:
            case Shader_Type_Unlit_Color:
                idsize = 16;
                idptr = &material->iData.unlitColor.color;
                glDisable(GL_BLEND);
                glEnable(GL_DEPTH_TEST);
                break;
            case Shader_Type_Gizmo:
                idsize = 16;
                idptr = &material->iData.gizmoMat.color;
                glDisable(GL_BLEND);
                glDisable(GL_DEPTH_TEST);
                break;
            case Shader_Type_Unlit_Fade:
                idsize = 16;
                idptr = &material->iData.unlitFade.color;
                tex = get_texture(renderer, material->iData.unlitFade.textureId);
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, tex->glTex);
                glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
                glEnable(GL_BLEND);
                glEnable(GL_DEPTH_TEST);
                break;
            default:
                assert(0);
        }

        for(instId = 0; instId < instanceCount; instId++, instRunId++)
        {
            RenderMeshInstance *minstance = &mentry->instances[instId];
            
            // buffer limit reached, have to flush
            if((idofst + idsize) > renderer->uniBufSize
                    || (mofst + sizeof(Mat4)) > renderer->uniBufSize)
            {
                opengl_flush_instances(renderer, mesh, material, instRunId, idbuf, idofst, mbuf, oidBuf);
                idofst = mofst = instRunId = 0;
            }

            PROF_START_STR("Copy instance data");
            if(idsize > 0)
                memcpy(&idbuf[idofst], idptr, idsize);
            if(renderer->renderObjectId)
                oidBuf[instRunId*4] = minstance->objectId;
            idofst += idsize;
            assert((idsize & 0xF) == 0); // TODO: std140 proper
            static_assert(sizeof(Mat4) == 4*4*4, "Mat4 assumed to be packed, otherwise it won't match OpenGL layout!");
            //@OPTIMIZE: could we somehow get rid of this copy?
            memcpy(&mbuf[mofst], &matBuf[minstance->matrixIndex], sizeof(Mat4));
            mofst += sizeof(Mat4);
            PROF_END();
        }
        // @OPTIMIZE: we currently reset buffer offset, but we could keep writing to
        // it until its full. the only thing we have to do is start writing from
        // where we were before
        if(idofst > 0 || mofst > 0)
        {
            opengl_flush_instances(renderer, mesh, material, instRunId, idbuf, idofst, mbuf, oidBuf);
            idofst = mofst = instRunId = 0;
        }
    }

    PROF_START_STR("Render Immediate triangles");
    // for some reason borders are wound backwards in nuklear
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendEquation(GL_FUNC_ADD);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    uint32_t vertBufSize = sizeof(*view->vertices) * view->numVertices;
    uint32_t idxBufSize = sizeof(*view->indices) * view->numIndices;
    if(vertBufSize > 0 && idxBufSize > 0)
    {
        assert(renderer->immVertBufSize >= vertBufSize);
        assert(renderer->immIndexBufSize >= idxBufSize);
        glBindVertexArray(renderer->immVAO);
        glBindBuffer(GL_ARRAY_BUFFER, renderer->immVertBuf);
        glBufferSubData(GL_ARRAY_BUFFER, 0, vertBufSize, view->vertices);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, renderer->immIndexBuf);
        glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, idxBufSize, view->indices);
        glUseProgram(renderer->uiProgram);        
        GLuint loc = glGetUniformLocation(renderer->uiProgram, "toClip");
        glUniformMatrix4fv(loc, 1, GL_FALSE, (GLfloat*)&view->orthoMatrix);
        loc = glGetUniformLocation(renderer->uiProgram, "_MainTex");
        glUniform1i(loc, 0);
        for(int i = 0; i < view->numUIBatches; i++)
        {
            UIBatch *ub = &view->uiBatches[i];
            glEnable(GL_SCISSOR_TEST);
            glScissor(ub->scissorX0, ub->scissorY0, ub->scissorX1, ub->scissorY1);

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, renderer->textures[ub->textureId].glTex);
            GLvoid * offset = (GLvoid*)(uintptr_t)(ub->indexStart*sizeof(view->indices[0]));
            glDrawElements(GL_TRIANGLES, ub->indexCount, GL_UNSIGNED_SHORT, offset);
            assert((ub->indexCount % 3) == 0);
            assert(ub->indexStart + ub->indexCount <= view->numIndices);
        }
    }
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glDisable(GL_SCISSOR_TEST);
    PROF_END();
}

internal void opengl_init_object_id_buffer(OpenGLRenderer *renderer)
{
    assert(renderer->objectIdPbo == 0);
    glGenBuffers(1, &renderer->objectIdPbo);
    glBindBuffer(GL_PIXEL_PACK_BUFFER, renderer->objectIdPbo);
    glBufferData(GL_PIXEL_PACK_BUFFER, renderer->fboWidth*renderer->fboHeight*4, NULL, GL_DYNAMIC_READ);
    glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);

    assert(renderer->objectIdInstanceBuf == 0);
    glGenBuffers(1, &renderer->objectIdInstanceBuf);
    glBindBuffer(GL_UNIFORM_BUFFER, renderer->objectIdInstanceBuf);
    glBufferData(GL_UNIFORM_BUFFER, renderer->uniBufSize, 0, GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_UNIFORM_BUFFER, UNIFORM_BUFFER_OBJECTID, renderer->objectIdInstanceBuf);

    glGenTextures(1, &renderer->objectIdFboTex);
    glBindTexture(GL_TEXTURE_2D, renderer->objectIdFboTex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R32I, renderer->fboWidth, renderer->fboHeight, 0, GL_RED_INTEGER, GL_INT, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, renderer->fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, renderer->objectIdFboTex, 0);
    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    assert(status == GL_FRAMEBUFFER_COMPLETE);
    renderer->renderObjectId = true;

    glBindTexture(GL_TEXTURE_2D, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

internal void opengl_destroy_object_id_buffer(OpenGLRenderer *renderer)
{
    assert(renderer->objectIdPbo != 0);
    assert(renderer->objectIdInstanceBuf != 0);
    glDeleteBuffers(1, &renderer->objectIdPbo);
    glDeleteBuffers(1, &renderer->objectIdInstanceBuf);
    renderer->objectIdPbo = 0;
    renderer->objectIdInstanceBuf = 0;
    glBindFramebuffer(GL_FRAMEBUFFER, renderer->fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, 0, 0);
    glDeleteTextures(1, &renderer->objectIdFboTex);
    renderer->renderObjectId = false;
}

void *opengl_update_proc(void *data);

GLuint opengl_create_standard_program_noerror(const char *vertSrc, const char *fragSrc)
{
    GLuint vs, fs, program;
    GLint linkstatus;
    GLint vsLen, fsLen;
    vs = glCreateShader(GL_VERTEX_SHADER);
    fs = glCreateShader(GL_FRAGMENT_SHADER);
    vsLen = strlen(vertSrc);
    fsLen = strlen(fragSrc);
    glShaderSource(vs, 1, &vertSrc, &vsLen);
    glShaderSource(fs, 1, &fragSrc, &fsLen);
    glCompileShader(vs);
    glCompileShader(fs);
    program = glCreateProgram();
    glAttachShader(program, vs);
    glAttachShader(program, fs);
    glBindAttribLocation(program, 0, "a_position");
    glBindAttribLocation(program, 1, "a_user0");
    glBindAttribLocation(program, 2, "a_user1");
    glBindAttribLocation(program, 3, "a_user2");
    glBindAttribLocation(program, 4, "a_user3");
    glLinkProgram(program);
    glDetachShader(program, vs);
    glDetachShader(program, fs);

    glGetProgramiv(program, GL_LINK_STATUS, &linkstatus);
    if(linkstatus == GL_FALSE)
    {
        char errorBuf[1024];
        glGetProgramInfoLog(program, 1024, NULL, errorBuf);
        fprintf(stderr, 
                "OpenGL Failed to link program:\n"
                "------------ OPENGL PROGRAM LINK LOG ---------------\n"
                "%s\n"
                "------------ END OF REPORT -------------------------\n"
                , errorBuf);
        assert(0);
    }

    return program;
}

internal void opengl_init_builtin_shaders(OpenGLRenderer *renderer)
{
   GLuint program;
   program = opengl_create_standard_program_noerror(fallbackVert, fallbackFrag);
   renderer->builtinPrograms[Shader_Type_None] = program;
   program = opengl_create_standard_program_noerror(vertShaderSrc, fragShaderSrc);
   renderer->builtinPrograms[Shader_Type_Unlit_Textured] = program;
   renderer->builtinPrograms[Shader_Type_Unlit_Fade] = program;
   program = opengl_create_standard_program_noerror(vertShaderSrc, fragShaderCutoutSrc);
   renderer->builtinPrograms[Shader_Type_Unlit_Textured_Cutout] = program;
   program = opengl_create_standard_program_noerror(vertShaderSrc, fragShaderSolidSrc);
   renderer->builtinPrograms[Shader_Type_Unlit_Vertex_Color] = program;
   renderer->builtinPrograms[Shader_Type_Unlit_Color] = program; // TODO?
   program = opengl_create_standard_program_noerror(gizmoVertSrc, gizmoFragSrc);
   renderer->builtinPrograms[Shader_Type_Gizmo] = program;
   program = opengl_create_standard_program_noerror(uiVertSrc, uiFragSrc);
   renderer->uiProgram = program;
}

internal void opengl_destroy_builtin_shaders(OpenGLRenderer *renderer)
{
    glDeleteProgram(renderer->builtinPrograms[Shader_Type_None]);
    glDeleteProgram(renderer->builtinPrograms[Shader_Type_Unlit_Textured]);
    glDeleteProgram(renderer->builtinPrograms[Shader_Type_Unlit_Vertex_Color]);
    glDeleteProgram(renderer->builtinPrograms[Shader_Type_Gizmo]);
    glDeleteProgram(renderer->uiProgram);
}

internal void opengl_init(OpenGLRenderer *renderer)
{
    // Set expected state
    //

    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    // TODO: does this require any extensions?
#ifndef AIKE_GLES
    glEnable(GL_FRAMEBUFFER_SRGB);
#else
#warning no GL_FRAMEBUFFER_SRGB on GLES
#endif
    // glDrawBuffer(GL_BACK);


    // Get and check available resources
    // Init default values
    //

    GLint maxUniBindings;
    glGetIntegerv(GL_MAX_UNIFORM_BUFFER_BINDINGS, &maxUniBindings);
    if(UNIFORM_BUFFER_COUNT > maxUniBindings)
    {
        fprintf(stderr, "OpenGL: max uniform buffer bindings supported %d, but need at least %d\n", maxUniBindings, UNIFORM_BUFFER_COUNT);
        //exit(-1);
    }
    renderer->objectIdPbo = 0;
    renderer->objectIdInstanceBuf = 0;
    renderer->renderObjectId = false;

    // Generate default framebuffer
    //

    // generate color texture
    uint32_t width = renderer->renderer.platform->mainWin.width;
    uint32_t height = renderer->renderer.platform->mainWin.height;
    renderer->fboWidth = width;
    renderer->fboHeight = height;
    printf("fbo %d %d\n", renderer->fboWidth, renderer->fboHeight);
    glGenTextures(1, &renderer->fboColorTex);
    glBindTexture(GL_TEXTURE_2D, renderer->fboColorTex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    // why do opengl docs tell to use GL_BGRA?
    //glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB8_ALPHA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    // generate fbo and attach color
    glGenFramebuffers(1, &renderer->fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, renderer->fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, renderer->fboColorTex, 0);
    // generate depth buffer and attach it
    glGenRenderbuffers(1, &renderer->fboDepthBuffer);
    glBindRenderbuffer(GL_RENDERBUFFER, renderer->fboDepthBuffer);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, width, height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, renderer->fboDepthBuffer);
    const GLenum buffers[] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1};
    glDrawBuffers(2, buffers);
    // check status
    GLenum status;
    status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    assert(status == GL_FRAMEBUFFER_COMPLETE);

    // Generate mesh instance data buffers
    //

    GLint ibufsize;
    glGetIntegerv(GL_MAX_UNIFORM_BLOCK_SIZE, &ibufsize);
    renderer->uniBufSize = ibufsize;

    glGenBuffers(INSTANCE_BUFFER_COUNT, renderer->instanceDataBuffers);
    for(int i = 0; i < INSTANCE_BUFFER_COUNT; i++)
    {
        glBindBuffer(GL_UNIFORM_BUFFER, renderer->instanceDataBuffers[i]);
        glBufferData(GL_UNIFORM_BUFFER, renderer->uniBufSize, 0, GL_DYNAMIC_DRAW);
        glBindBufferBase(GL_UNIFORM_BUFFER, UNIFORM_BUFFER_INSTANCEDATA+i, renderer->instanceDataBuffers[i]);
    }
    renderer->curInstanceBufferIdx = 0;

    glGenBuffers(MATRIX_BUFFER_COUNT, renderer->matrixBuffers);
    for(int i = 0; i < MATRIX_BUFFER_COUNT; i++)
    {
        glBindBuffer(GL_UNIFORM_BUFFER, renderer->matrixBuffers[i]);
        glBufferData(GL_UNIFORM_BUFFER, renderer->uniBufSize, 0, GL_DYNAMIC_DRAW);
        glBindBufferBase(GL_UNIFORM_BUFFER, UNIFORM_BUFFER_MATRICES+i, renderer->matrixBuffers[i]);
    }
    renderer->curMatrixBufferIdx = 0;

    // Generate immediate mode buffers (UI)
    //

    // TODO: enough?
    glGenVertexArrays(1, &renderer->immVAO);
    glBindVertexArray(renderer->immVAO);
    renderer->immVertBufSize = 1024*1024;
    renderer->immIndexBufSize = 512*1024;
    glGenBuffers(1, &renderer->immVertBuf);
    glBindBuffer(GL_ARRAY_BUFFER, renderer->immVertBuf);
    glBufferData(GL_ARRAY_BUFFER, renderer->immVertBufSize, 0, GL_DYNAMIC_DRAW);
    glGenBuffers(1, &renderer->immIndexBuf); 
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, renderer->immIndexBuf);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, renderer->immIndexBufSize, 0, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(UIVertex), (GLvoid*)offsetof(UIVertex, position));
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(UIVertex), (GLvoid*)offsetof(UIVertex, texCoord));
    // TODO: f32 colors?
    glVertexAttribPointer(2, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(UIVertex), (GLvoid*)offsetof(UIVertex, color));
    glBindVertexArray(0);

    // Generate default mesh, texture and material
    // and allocate slots for resources
    //

    renderer->meshes = NULL;
    renderer->materials = NULL;
    renderer->textures = NULL;

    uint32_t white = 0xFFFFFFFF;
    GLuint whiteTex;
    glGenTextures(1, &whiteTex);
    glBindTexture(GL_TEXTURE_2D, whiteTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, &white);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    GLuint bufs[2];
    GLuint vao;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    glGenBuffers(2, bufs);
    glBindBuffer(GL_ARRAY_BUFFER, bufs[0]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(glDefaultMesh), glDefaultMesh, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bufs[1]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(glDefaultMeshIdx), glDefaultMeshIdx, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 32, (GLvoid*)(uintptr_t)0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 32, (GLvoid*)(uintptr_t)16);
    glVertexAttribPointer(2, 4, GL_UNSIGNED_BYTE, GL_TRUE, 32, (GLvoid*)(uintptr_t)28);
    glBindVertexArray(0);

    opengl_init_builtin_shaders(renderer);

    // TODO: if any of these get reallocated we're screwed!!!!!!!!!!!!!!!!!!!!!!!!!!
    // USE A FUCKING POOL
    buf_reserve(renderer->meshes, 1000);
    buf_reserve(renderer->textures, 1000);
    buf_reserve(renderer->materials, 1000);

    GLMesh mesh = {
        .id = 0,
        .state = GL_Mesh_State_Ready,
        .numVertices = ARRAY_COUNT(glDefaultMesh),
        .numIndices = ARRAY_COUNT(glDefaultMeshIdx),
        .glVbo = bufs[0],
        .glEbo = bufs[1],
        .glVao = vao,
        .vertexStride = 32,
    };

    GLTexture tex = {
        .id = 0,
        .state = GL_Texture_State_Ready,
        .format = Texture_Format_RGBA,
        .filter = Texture_Filter_None,
        .width = 1,
        .height = 1,
        .glTex = whiteTex,
        .glPub = 0,
        .bufferSize = 0
    };

    GLMaterial mat = { 
        .id = 0,
        .shaderId = Shader_Type_Unlit_Vertex_Color,
        .glProgram = renderer->builtinPrograms[Shader_Type_Unlit_Vertex_Color],
        .userData = NULL,
        .perInstanceDataSize = sizeof(struct UnlitVertexColor),
        .iData.unlitVertexColor.color = (V4){1.0f, 1.0f, 1.0f, 1.0f},
    };
    buf_push(renderer->materials, mat);

    mat = (GLMaterial){ 
        .id = 1,
        .shaderId = Shader_Type_Gizmo,
        .glProgram = renderer->builtinPrograms[Shader_Type_Gizmo],
        .userData = NULL,
        .perInstanceDataSize = sizeof(struct GizmoMat),
        .iData.gizmoMat.color = (V4){1.0f, 0.0f, 0.0f, 1.0f},
    };
    buf_push(renderer->materials, mat);
    mat = (GLMaterial){ 
        .id = 2,
        .shaderId = Shader_Type_Gizmo,
        .glProgram = renderer->builtinPrograms[Shader_Type_Gizmo],
        .userData = NULL,
        .perInstanceDataSize = sizeof(struct GizmoMat),
        .iData.gizmoMat.color = (V4){0.0f, 1.0f, 0.0f, 1.0f},
    };
    buf_push(renderer->materials, mat);
    mat = (GLMaterial){ 
        .id = 3,
        .shaderId = Shader_Type_Gizmo,
        .glProgram = renderer->builtinPrograms[Shader_Type_Gizmo],
        .userData = NULL,
        .perInstanceDataSize = sizeof(struct GizmoMat),
        .iData.gizmoMat.color = (V4){0.0f, 0.0f, 1.0f, 1.0f},
    };
    buf_push(renderer->materials, mat);


    buf_push(renderer->textures, tex);
    buf_push(renderer->meshes, mesh);

    // TODO: remove
    opengl_init_object_id_buffer(renderer);
}

void check_sync_points(OpenGLRenderer *renderer)
{ 
    for(int i = 0; i < GL_RENDERER_MAX_SYNC_POINTS; i++)
    {
        // TODO: it would probably be cheaper to keep a list than always iterate all
        GLSyncPoint *syncp = &renderer->syncPoints[i];
        if(syncp->active)
        {
            // TODO: i changed syncp->sync to a value type, but if the fence is update or whatever it might change or whatever
            GLenum waitsync = glClientWaitSync(syncp->sync, 0, 0);
            assert(waitsync != GL_WAIT_FAILED);
            if(waitsync == GL_ALREADY_SIGNALED || waitsync == GL_CONDITION_SATISFIED)
            {
                opengl_trigger_sync(renderer, i);
            }
        }
    }
}

internal void *opengl_proc(void *data);

// MAIN THREAD CALLS THIS
Renderer *create_opengl_renderer(AikePlatform *platform)
{
    // TODO: proper allocator (caller frees this right now)
    OpenGLRenderer *ret = aligned_alloc(_Alignof(OpenGLRenderer), sizeof(OpenGLRenderer));

    ret->renderer.platform = platform;
    ret->curView = NULL;

    platform->make_window_current(platform, &platform->mainWin);
    opengl_init(ret);

    Renderer *grenderer = (Renderer*)ret;

    for(int i = 0 ; i< GL_RENDERER_MAX_SYNC_POINTS; i++)
    {
        ret->syncPointFreeList[i] = i;
        ret->syncPoints[i] = (GLSyncPoint){};
    }
    ret->numFreeSyncPoints = GL_RENDERER_MAX_SYNC_POINTS;

    return (Renderer*)ret;
}

void stop_opengl_renderer(Renderer *rend)
{
    if(NULL == rend->renderThread)
    {
        fprintf(stderr, "stop_opengl_renderer when not started!\n");
        return;
    }
    RenderMessage msg;
    msg.type = Render_Message_Stop;
    renderer_queue_message(rend, &msg);

    OpenGLRenderer *glrend = (OpenGLRenderer*)rend;

    void *result;
    rend->platform->join_thread(rend->renderThread, &result);
    printf("render thread exited with %p\n", result);
    // TODO: does this leak?
    rend->renderThread = NULL;
}

void start_opengl_renderer(Renderer *rend)
{
    if(NULL != rend->renderThread)
    {
        fprintf(stderr, "start_opengl_renderer when already started!\n");
        return;
    }

    rend->renderThread = rend->platform->create_thread(rend, opengl_proc);
}

// MAIN THREAD CALLS THIS
void destroy_opengl_renderer(Renderer *rend)
{
    OpenGLRenderer *glrend = (OpenGLRenderer*)rend;

    // TODO: just move this to render thread...
    rend->platform->make_window_current(rend->platform, &rend->platform->mainWin);
    glDeleteBuffers(INSTANCE_BUFFER_COUNT, glrend->instanceDataBuffers);
    glDeleteBuffers(MATRIX_BUFFER_COUNT, glrend->matrixBuffers);
    glDeleteBuffers(1, &glrend->immVertBuf);
    glDeleteBuffers(1, &glrend->immVertBuf);
    glDeleteVertexArrays(1, &glrend->immVAO);

    // default resources are not deleted because ideally they will be deleted
    // like any other resource, which we currently do not do
    
    if(glrend->objectIdPbo != 0)
        opengl_destroy_object_id_buffer(glrend);

    // delete default fbo
    glDeleteFramebuffers(1, &glrend->fbo);
    glDeleteTextures(1, &glrend->fboColorTex);
    glDeleteRenderbuffers(1, &glrend->fboDepthBuffer);

    opengl_destroy_builtin_shaders(glrend);

    buf_free(glrend->meshes);
    buf_free(glrend->textures);
    buf_free(glrend->materials);
}

internal void *opengl_proc(void *data)
{
    // TODO: new debug context is created for every renderer_start
    DEBUG_INIT("OpenGL thread");
    OpenGLRenderer *renderer = (OpenGLRenderer*)data;
    SwapBuffer *viewSwapBuffer = ((Renderer*)data)->swapBuffer;
    if(renderer->curView == NULL)
        renderer->curView = take_view_buffer(viewSwapBuffer);
    assert(renderer->curView);
    renderer->renderer.platform->make_window_current(renderer->renderer.platform, &renderer->renderer.platform->mainWin);
    bool running = true;
    while(running)
    {
        DEBUG_START_FRAME();

        // wait for new view, no point to render exact same view again
        // TODO: remove once you have motion vectors and stuff
        uintptr_t oldView = (uintptr_t)renderer->curView;
        while(oldView == (uintptr_t)renderer->curView)
        {
            running = opengl_process_messages(renderer);
            if(!running)
                break;
            check_sync_points(renderer);
            renderer->curView = swap_view_if_newer(viewSwapBuffer, renderer->curView);
            if(oldView == (uintptr_t)renderer->curView)
                renderer->renderer.platform->sleep(1000);
        }

        PROF_START_STR("opengl frame");

        PROF_START_STR("clear FBO");
        glBindFramebuffer(GL_FRAMEBUFFER, renderer->fbo);
        glViewport(0, 0, renderer->fboWidth, renderer->fboHeight);
        // TODO: clear color ? skybox?
        glClearColor(0.325f, 0.029f, 0.07f, 1.0f);
        glClear(GL_DEPTH_BUFFER_BIT);
        float cvalues[4] = {0.025f, 0.029f, 0.07f, 1.0f};
        int32_t values[4] = {-1, -1, -1, -1};
        glClearBufferfv(GL_COLOR, 0, cvalues);
        glClearBufferiv(GL_COLOR, 1, values);
        PROF_END();
        opengl_render_view(renderer, renderer->curView);

        PROF_START_STR("blit to backbuffer");
        glBindFramebuffer(GL_READ_FRAMEBUFFER, renderer->fbo);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
        // NOTE: currently we never resize backbuffer, so it is whatever size the renderer
        // was started with
        
        int x = renderer->curView->view.renderRect.x;
        int y = renderer->curView->view.renderRect.y;
        int w = renderer->curView->view.renderRect.z;
        int h = renderer->curView->view.renderRect.w;
        glBlitFramebuffer(0, 0, renderer->fboWidth, renderer->fboHeight, x, y, x+w, y+h, GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT, GL_NEAREST);

        // black bars
        glViewport(0, 0, renderer->windowWidth, renderer->windowHeight);
        glEnable(GL_SCISSOR_TEST);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        // black horizontal bars
        if(y > 0)
        {
            glScissor(0, 0, renderer->windowWidth, y);
            glClear(GL_COLOR_BUFFER_BIT);
            glScissor(0, y+h, renderer->windowWidth, y);
            glClear(GL_COLOR_BUFFER_BIT);
        }
        // black vertical bars
        if(x > 0)
        {
            glScissor(0, 0, x, renderer->windowHeight);
            glClear(GL_COLOR_BUFFER_BIT);
            glScissor(x+w, 0, x, renderer->windowHeight);
            glClear(GL_COLOR_BUFFER_BIT);
        }
        glDisable(GL_SCISSOR_TEST);

        PROF_END();

        PROF_START_STR("present frame");
        renderer->renderer.platform->present_frame(renderer->renderer.platform, &renderer->renderer.platform->mainWin);
        PROF_END();
        frameId++;

        PROF_END();
        DEBUG_END_FRAME();
        //DEBUG_FRAME_REPORT();
    }
    DEBUG_DESTROY();
    return (void*)0xFACEFEED;
}
