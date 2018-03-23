#include <GL/gl.h> // TODO: this should be part of platform

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
// update my value and keep me last for clarity!
#define UNIFORM_BUFFER_COUNT (1+MAX_INSTANCE_BUFFERS+MAX_MATRIX_BUFFERS)

static_assert(MAX_INSTANCE_BUFFERS >= INSTANCE_BUFFER_COUNT, "Instance buffer count can't be larger than MAX_INSTANCE_BUFFERS, consider increasing both.");
static_assert(UNIFORM_BUFFER_COUNT <= 36, "OpenGL does not guarantee more than 36 uniform buffer bindings, thus some hardware might not support it");

GLenum shader_type_to_glenum(u32 type)
{
    switch(type)
    {
        case ShaderType_GLSL_Vert:
            return GL_VERTEX_SHADER;
        case ShaderType_GLSL_Frag:
            return GL_FRAGMENT_SHADER;
        default:
            assert(false);
            break;
    }
    return 0;
}

const char *opengl_type_to_str(GLint type)
{
    // no worries i used vim macro
    switch(type)
    {
    case GL_FLOAT: 	return "float";
    case GL_FLOAT_VEC2: 	return "vec2";
    case GL_FLOAT_VEC3: 	return "vec3";
    case GL_FLOAT_VEC4: 	return "vec4";
    case GL_DOUBLE: 	return "double";
    case GL_DOUBLE_VEC2: 	return "dvec2";
    case GL_DOUBLE_VEC3: 	return "dvec3";
    case GL_DOUBLE_VEC4: 	return "dvec4";
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
    case GL_DOUBLE_MAT2: 	return "dmat2";
    case GL_DOUBLE_MAT3: 	return "dmat3";
    case GL_DOUBLE_MAT4: 	return "dmat4";
    case GL_DOUBLE_MAT2x3: 	return "dmat2x3";
    case GL_DOUBLE_MAT2x4: 	return "dmat2x4";
    case GL_DOUBLE_MAT3x2: 	return "dmat3x2";
    case GL_DOUBLE_MAT3x4: 	return "dmat3x4";
    case GL_DOUBLE_MAT4x2: 	return "dmat4x2";
    case GL_DOUBLE_MAT4x3: 	return "dmat4x3";
    case GL_SAMPLER_1D: 	return "sampler1D";
    case GL_SAMPLER_2D: 	return "sampler2D";
    case GL_SAMPLER_3D: 	return "sampler3D";
    case GL_SAMPLER_CUBE: 	return "samplerCube";
    case GL_SAMPLER_1D_SHADOW: 	return "sampler1DShadow";
    case GL_SAMPLER_2D_SHADOW: 	return "sampler2DShadow";
    case GL_SAMPLER_1D_ARRAY: 	return "sampler1DArray";
    case GL_SAMPLER_2D_ARRAY: 	return "sampler2DArray";
    case GL_SAMPLER_1D_ARRAY_SHADOW: 	return "sampler1DArrayShadow";
    case GL_SAMPLER_2D_ARRAY_SHADOW: 	return "sampler2DArrayShadow";
    case GL_SAMPLER_2D_MULTISAMPLE: 	return "sampler2DMS";
    case GL_SAMPLER_2D_MULTISAMPLE_ARRAY: 	return "sampler2DMSArray";

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
    case GL_INT_SAMPLER_2D_MULTISAMPLE: 	return "isampler2DMS";
    case GL_INT_SAMPLER_2D_MULTISAMPLE_ARRAY: 	return "isampler2DMSArray";
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
    default: return "unknown";
    }
}

void opengl_process_program(GLuint program, struct Material *material)
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
        printf("%s %s blockId: %d Offset: %d Stride: %d Size: %d\n", buf, opengl_type_to_str(resultTypes[i]), resultBlocks[i], resultOffsets[i], resultStrides[i], resultSizes[i]);
    }
    printf("---- end ----\n\n");

    // find the size of istance data struct
    GLuint instanceBlock = glGetUniformBlockIndex(program, "instanceData");
    u32 instanceDataSize = 0;
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
                        curOffset = ALIGN4(curOffset);
                        curOffset += 4;
                        break;
                    case GL_FLOAT_VEC2:
                    case GL_INT_VEC2:
                    case GL_UNSIGNED_INT_VEC2:
                        curOffset = ALIGN8(curOffset);
                        curOffset += 8;
                        break;
                    case GL_FLOAT_VEC3:
                    case GL_INT_VEC3:
                    case GL_UNSIGNED_INT_VEC3:
                        curOffset = ALIGN16(curOffset);
                        curOffset += 12;
                        break;
                    case GL_FLOAT_VEC4:
                    case GL_INT_VEC4:
                    case GL_UNSIGNED_INT_VEC4:
                        curOffset = ALIGN16(curOffset);
                        curOffset += 16;
                        break;
                    case GL_FLOAT_MAT4:
                        curOffset = ALIGN16(curOffset);
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
        material->perInstanceDataSize = ALIGN16(curOffset);
        printf("expected offset for next: %d\n", material->perInstanceDataSize);
    }
    else
    {
        printf("WARNING: instanceData uniform block not found in shader!\n");
        material->perInstanceDataSize = 0;
    }
}

static inline void opengl_change_instance_buffer(struct OpenGLRenderer *renderer)
{
    renderer->curInstanceBufferIdx = ++renderer->curInstanceBufferIdx%INSTANCE_BUFFER_COUNT;
}

void opengl_flush_instances(struct OpenGLRenderer *renderer, struct Mesh *mesh, struct Material *mat, u32 instanceCount, void *uniformData, u32 uniformDataSize, void *matData)
{
    PROF_BLOCK();
    u32 bufIdx = renderer->curInstanceBufferIdx;
    glBindBuffer(GL_UNIFORM_BUFFER, renderer->instanceDataBuffers[bufIdx]);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, uniformDataSize, uniformData);

    // prepare mesh, program and uniforms
    GLuint program = (GLuint)mat->rendererHandle;
    glUseProgram(program);
    GLuint blockIdx = glGetUniformBlockIndex(program, "instanceBlock");
    if(blockIdx != GL_INVALID_INDEX)
        glUniformBlockBinding(program, blockIdx, UNIFORM_BUFFER_INSTANCEDATA+bufIdx);

    bufIdx = renderer->curMatrixBufferIdx;
    glBindBuffer(GL_UNIFORM_BUFFER, renderer->matrixBuffers[bufIdx]);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, instanceCount*sizeof(struct Mat4), matData);
    blockIdx = glGetUniformBlockIndex(program, "matrixBlock");
    if(blockIdx != GL_INVALID_INDEX)
        glUniformBlockBinding(program, blockIdx, UNIFORM_BUFFER_MATRICES+bufIdx);

    // TODO
    if(mesh->tex != NULL)
    {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, mesh->tex->rendererHandle);
    }
    glBindVertexArray(mesh->rendererHandle3);

    glDrawElementsInstanced(GL_TRIANGLES, mesh->numIndices, GL_UNSIGNED_SHORT, 0, instanceCount);

    /*GLuint instIDU = glGetUniformLocation(program, "instId");
    for(u32 j = 0; j < instanceCount; j++)
    {
        glUniform1i(instIDU, j);
        glDrawElements(GL_TRIANGLES, mesh->numIndices, GL_UNSIGNED_SHORT, 0);
    }*/

    opengl_change_instance_buffer(renderer);
}

void calculate_vertex_buffer_size(struct MeshQuery *mq, uint32_t *size, uint32_t *stride, uint32_t offsetArr[])
{
    uint32_t vertexCount = mq->vertexCount;
    uint32_t perVertexSize = sizeof(struct V4);
    for(int i = 0; i < MAX_ATTRIBUTE_BUFFERS; i++)
    {
        // TODO: handle invalid type
        offsetArr[i] = perVertexSize;
        perVertexSize += s_vertexAttributeTypeSizes[mq->attributeTypes[i]];
    }
    *size = vertexCount * perVertexSize;
    *stride = perVertexSize;
}

void opengl_handle_mesh_query(struct OpenGLRenderer *renderer, struct MeshQuery *mq)
{
    uint32_t meshId = mq->meshId;
    if(meshId == 0)
    {
        struct Mesh mesh = {};
        buf_push(renderer->renderer.meshes, mesh);
        meshId = buf_len(renderer->renderer.meshes) - 1;
        printf("new mesh %d\n", meshId);
    }
    else if(meshId >= buf_len(renderer->renderer.meshes))
    {
        // TODO: log error
        return;
    }

    uint32_t vertBufSize, vertexStride;
    uint32_t attribOffsets[MAX_ATTRIBUTE_BUFFERS];
    calculate_vertex_buffer_size(mq, &vertBufSize, &vertexStride, attribOffsets);
    uint32_t indexBufSize = mq->indexCount * (mq->largeIndices?4:2);
    if(vertBufSize == 0 || indexBufSize == 0)
    {
        // TODO: log error
        return;
    }

    struct Mesh *mesh = &renderer->renderer.meshes[meshId];

    GLuint vao = mesh->rendererHandle3;
    if(vao == 0)
    {
        glGenVertexArrays(1, &vao);
        mesh->rendererHandle3 = (uintptr_t)vao;
    }
    glBindVertexArray(vao);

    GLuint vbo = (GLuint)mesh->rendererHandle;
    if(vbo == 0)
    {
        glGenBuffers(1, &vbo);
        mesh->rendererHandle = (uintptr_t)vbo;
    }
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    if(mesh->vertexBufferSize < vertBufSize)
    {
        glBufferData(GL_ARRAY_BUFFER, vertBufSize, 0, GL_STATIC_DRAW);
        mesh->vertexBufferSize = vertBufSize;
    }
    void *vertPtr = glMapBufferRange(GL_ARRAY_BUFFER, 0, vertBufSize, GL_MAP_WRITE_BIT | GL_MAP_UNSYNCHRONIZED_BIT);

    GLuint ebo = (GLuint)mesh->rendererHandle2;
    if(ebo == 0)
    {
        glGenBuffers(1, &ebo);
        mesh->rendererHandle2 = (uintptr_t)ebo;
    }
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    if(mesh->indexBufferSize < indexBufSize)
    {
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexBufSize, 0, GL_STATIC_DRAW);
        mesh->indexBufferSize = indexBufSize;
    }
    void *indexPtr = glMapBufferRange(GL_ELEMENT_ARRAY_BUFFER, 0, indexBufSize, GL_MAP_WRITE_BIT | GL_MAP_UNSYNCHRONIZED_BIT);

    mesh->vertexStride = vertexStride;
    mesh->numVertices = mq->vertexCount;
    mesh->numIndices = mq->indexCount;
    memcpy(mesh->attribOffsets, &attribOffsets, sizeof(mesh->attribOffsets));
    memcpy(mesh->attribTypes, &mq->attributeTypes, sizeof(mesh->attribTypes));

    glBindVertexArray(0);

    RenderMessage msg = {};
    msg.type = Render_Message_Mesh_Query_Result;
    msg.meshQueryResult.meshId = meshId;
    msg.meshQueryResult.userData = mq->userData;
    msg.meshQueryResult.vertBufPtr = vertPtr;
    msg.meshQueryResult.idxBufPtr = indexPtr;
    msg.meshQueryResult.dataBufPtr = NULL; // TODO
    ring_queue_enqueue(RenderMessage, &renderer->renderer.ch.fromRenderer, &msg);
}

void opengl_handle_mesh_update(struct OpenGLRenderer *renderer, struct MeshUpdate *mu)
{
    // TODO: handle errors
    uint32_t meshId = mu->meshId;
    assert(meshId < buf_len(renderer->renderer.meshes));
    struct Mesh *mesh = &renderer->renderer.meshes[meshId];
    assert(mesh->rendererHandle != 0);
    assert(mesh->rendererHandle2 != 0);
    assert(mesh->rendererHandle3 != 0);
    GLuint vao, vbo, ebo;
    vao = (GLuint)mesh->rendererHandle3;
    vbo = (GLuint)mesh->rendererHandle;
    ebo = (GLuint)mesh->rendererHandle2;
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
            printf("attrib %d %d\n", i+1, mesh->attribOffsets[i]);
            glVertexAttribPointer(i+1, numfloats, GL_FLOAT, false, mesh->vertexStride, (GLvoid*)(uintptr_t)mesh->attribOffsets[i]);
        }
        else
            glDisableVertexAttribArray(i+1);
    }
    //glBindBuffer(GL_ARRAY_BUFFER, 0);
    //glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void opengl_destroy_material(struct Material *material)
{
    GLuint glshader;
    for(int i = 0; i < MATERIAL_MAX_SHADERS; i++)
    {
        uint32_t type = material->shaders[i].type;
        switch(type)
        {
            case ShaderType_GLSL_Vert:
            case ShaderType_GLSL_Frag:
                glshader = (GLuint)material->shaders[i].rendererHandle;
                if(glshader != 0)
                {
                    glDeleteShader(glshader);
                    material->shaders[i].rendererHandle = 0;
                }
                break;
            default:
                break;
        }
    }
    if(material->rendererHandle != 0)
    {
        glDeleteProgram((GLuint)material->rendererHandle);
        material->rendererHandle = 0;
    }
}

void opengl_handle_material_query(struct OpenGLRenderer *renderer, struct MaterialQuery *mq)
{
    uint32_t materialId = mq->materialId;
    if(materialId == 0)
    {
        struct Material mat = {};
        buf_push(renderer->renderer.materials, mat);
        materialId = buf_len(renderer->renderer.materials)-1;
    }
    struct Material *material = &renderer->renderer.materials[materialId];
    GLuint shadersToLink[MATERIAL_MAX_SHADERS];
    uint32_t numLink = 0;

    GLuint glshader;
    for(int i = 0; i < MATERIAL_MAX_SHADERS; i++)
    {
        uint32_t type = mq->shaderTypes[i];
        switch(type)
        {
            case ShaderType_GLSL_Vert:
            case ShaderType_GLSL_Frag:
                glshader = (GLuint)material->shaders[i].rendererHandle;
                if(glshader == 0)
                {
                    glshader = glCreateShader(type == ShaderType_GLSL_Vert 
                            ? GL_VERTEX_SHADER : GL_FRAGMENT_SHADER);
                    material->shaders[i].rendererHandle = (uintptr_t)glshader;
                }
                glShaderSource(glshader, 1, (const GLchar*const*)&mq->shaderCodes[i], (const GLint*)&mq->shaderLengths[i]);
                glCompileShader(glshader);
                shadersToLink[numLink++] = glshader;
                break;
            case ShaderType_None:
                glshader = (GLuint)material->shaders[i].rendererHandle;
                if(glshader != 0)
                {
                    glDeleteShader(glshader);
                    material->shaders[i].rendererHandle = 0;
                }
                break;
        }
    }
    GLuint program = (GLuint)material->rendererHandle;
    if(numLink > 0 && program == 0)
    {
        program = glCreateProgram();
        material->rendererHandle = (uintptr_t)program;
    }
    if(numLink > 0)
    {
        assert(numLink <= MATERIAL_MAX_SHADERS);
        for(int i = 0; i < numLink; i++)
        {
            assert(glIsShader(shadersToLink[i]));
            glAttachShader(program, shadersToLink[i]);
        }

        glBindAttribLocation(program, 0, "a_position");
        glBindAttribLocation(program, 1, "a_user0");
        glBindAttribLocation(program, 2, "a_user1");
        glBindAttribLocation(program, 3, "a_user2");
        glBindAttribLocation(program, 4, "a_user3");
        glLinkProgram(program);

        for(int i = 0; i < numLink; i++)
        {
            glDetachShader(program, shadersToLink[i]);
        }

        opengl_process_program(program, material);

        GLint linkStatus;
        glGetProgramiv(program, GL_LINK_STATUS, &linkStatus);
        if(linkStatus != GL_FALSE)
        {
            GLuint texLoc = glGetUniformLocation(program, "_MainTex");
            glUseProgram(program);
            glUniform1i(texLoc, 0);
            material->isValid = true;
            printf("successful link!\n");
        }
        else
        {
            char errorBuf[1024];
            glGetProgramInfoLog(program, 1024, NULL, errorBuf);
            fprintf(stderr, 
                    "OpenGL Failed to link program:\n"
                    "------------ OPENGL PROGRAM LINK LOG ---------------\n"
                    "%s\n"
                    "------------ END OF REPORT -------------------------\n"
                    , errorBuf);
            opengl_destroy_material(material);
            buf_pop_back(renderer->renderer.materials);
            //  TODO: log error
            return;
        }
    }
    else if(program != 0)
    {
        glDeleteProgram(program);
        material->rendererHandle = 0;
        material->isValid = false;
        opengl_destroy_material(material);
        buf_pop_back(renderer->renderer.materials);
        // TODO: log error
        return;
    }

    RenderMessage msg = {};
    msg.type = Render_Message_Material_Ready;
    msg.materialQueryDone.materialId = materialId;
    msg.materialQueryDone.userData = mq->userData;
    ring_queue_enqueue(RenderMessage, &renderer->renderer.ch.fromRenderer, &msg);
}

void opengl_handle_texture_query(struct OpenGLRenderer *renderer, struct TextureQuery *tq)
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
        struct Texture tex = {};
        buf_push(renderer->renderer.textures, tex);
        textureId = buf_len(renderer->renderer.textures)-1;
    }
    struct Texture *tex = &renderer->renderer.textures[textureId];
    // TODO: check overflow for this, materials and meshes

    GLuint pbo = (GLuint)tex->rendererHandle2;
    if(pbo == 0)
    {
        glGenBuffers(1, &pbo);
        tex->rendererHandle2 = (uintptr_t)pbo;
    }
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbo);

    assert(tq->format == Texture_Format_RGBA);
    uint32_t requiredSize = 4*tq->width*tq->height;
    if(tex->bufferSize < requiredSize)
    {
        glBufferData(GL_PIXEL_UNPACK_BUFFER, requiredSize, NULL, GL_STATIC_DRAW);
        tex->bufferSize = requiredSize;
    }
    void *dataPtr = glMapBufferRange(GL_PIXEL_UNPACK_BUFFER, 0, requiredSize, GL_MAP_WRITE_BIT | GL_MAP_UNSYNCHRONIZED_BIT);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);

    tex->width = tq->width;
    tex->height = tq->height;
    tex->format = tq->format;

    RenderMessage response = {};
    response.type = Render_Message_Texture_Query_Response;
    response.texQR.textureId = textureId;
    response.texQR.userData = tq->userData;
    response.texQR.textureDataPtr = dataPtr;
    ring_queue_enqueue(RenderMessage, &renderer->renderer.ch.fromRenderer, &response);
}

void opengl_handle_texture_update(struct OpenGLRenderer *renderer, struct TextureUpdate *tu)
{
    uint32_t textureId = tu->textureId;
    if(textureId == 0)
    {
        // TODO: log error
        fprintf(stderr, "NOT A TEXTURE\n");
        return;
    }
    struct Texture *tex = &renderer->renderer.textures[textureId];
    // TODO: bound check
    // TODO: state check
    GLuint pbo = (GLuint)tex->rendererHandle2;
    if(pbo == 0)
    {
        // TODO: log error
        fprintf(stderr, "NO PBO\n");
        return;
    }
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbo);
    glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);

    GLuint gltex = (GLuint)tex->rendererHandle;
    if(gltex == 0)
    {
        glGenTextures(1, &gltex);
        tex->rendererHandle = (uintptr_t)gltex;
    }
    glBindTexture(GL_TEXTURE_2D, gltex);
    assert(tex->format = Texture_Format_RGBA); // TODO: more formats
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tex->width, tex->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glGenerateMipmap(GL_TEXTURE_2D);

    //glBindTexture(GL_TEXTURE_2D, 0);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
}

void opengl_process_messages(struct OpenGLRenderer *renderer)
{
    struct Renderer *r = &renderer->renderer;
    RenderMessage msg;
    while(ring_queue_dequeue(RenderMessage, &r->ch.toRenderer, &msg))
    {
        switch(msg.type)
        {
            case Render_Message_Mesh_Query:
                printf("OpenGL received mesh query!\n");
                opengl_handle_mesh_query(renderer, &msg.meshQuery);
                break;
            case Render_Message_Mesh_Update:
                printf("OpenGL received mesh update!\n");
                opengl_handle_mesh_update(renderer, &msg.meshUpdate);
                break;
            case Render_Message_Material_Query:
                printf("OpenGL received material query!\n");
                opengl_handle_material_query(renderer, &msg.materialQuery);
                break;
            case Render_Message_Texture_Query:
                printf("OpenGL recived texture query!\n");
                opengl_handle_texture_query(renderer, &msg.texQ);
                break;
            case Render_Message_Texture_Update:
                printf("OpenGL received texture update!\n");
                opengl_handle_texture_update(renderer, &msg.texU);
                break;
            default:
                fprintf(stderr, "OpenGL renderer: unknown message type!\n");
                break;
        }
    }
}

void opengl_render_view(struct OpenGLRenderer *renderer, struct RenderViewBuffer *rbuf)
{
    // TODO: remove once you have separate thread for this
    opengl_process_messages(renderer);

    PROF_BLOCK();
    // TODO: clear color ? skybox?
    glClearColor(0.6f, 0.2f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    struct RenderView *view = &rbuf->view;
    u32 meshCount = view->space->numEntries;
    
    // TODO: temp allocators
    u8 idbuf[renderer->uniBufSize];
    u8 mbuf[renderer->uniBufSize];
    u32 idofst;
    u32 mofst;

    for(u32 i = 0; i < meshCount; i++)
    {
        idofst = 0;
        mofst = 0;
        struct RenderMeshEntry *mentry = view->space->meshEntries[i];
        struct Material *material = &renderer->renderer.materials[mentry->materialId];
        struct Mesh *mesh = &renderer->renderer.meshes[mentry->meshId];
        // use fallback material if the one attached to mesh isn't valid
        if(!material->isValid)
            material = &renderer->renderer.materials[0];

        u32 instanceCount = mentry->numInstances;
        assert(instanceCount != 0);

        u32 instId = 0;
        u32 instRunId = 0;

        for(instId = 0; instId < instanceCount; instId++, instRunId++)
        {
            struct RenderMeshInstance *minstance = &mentry->instances[instId];
            u32 idsize = minstance->instanceDataSize;
            void *idptr= minstance->instanceDataPtr;
            // TODO: std140 rules
            assert((idsize & 0xF) == 0);

            // buffer limit reached, have to flush
            if((idofst + idsize) > renderer->uniBufSize
                    || (mofst + sizeof(struct Mat4)) > renderer->uniBufSize)
            {
                opengl_flush_instances(renderer, mesh, material, instRunId, idbuf, idofst, mbuf);
                idofst = mofst = instRunId = 0;
            }

            memcpy(&idbuf[idofst], idptr, idsize);
            idofst += idsize;
            static_assert(sizeof(struct Mat4) == 4*4*4, "Mat4 assumed to be packed, otherwise it won't match OpenGL layout!");
            //@OPTIMIZE: could extract 4 at a time?
            u32 mmidx = minstance->matrixIndex;
            mat4_extract_sse2((struct Mat4*)&mbuf[mofst], &view->tmatrixBuf[mmidx>>2], mmidx&0x3);
            mofst += sizeof(struct Mat4);
        }
        // @OPTIMIZE: we currently reset buffer offset, but we could keep writing to
        // it until its full. the only thing we have to do is start writing from
        // where we were before
        if(idofst > 0 || mofst > 0)
        {
            opengl_flush_instances(renderer, mesh, material, instRunId, idbuf, idofst, mbuf);
            idofst = mofst = instRunId = 0;
        }
    }
}

struct Renderer *create_opengl_renderer()
{
    // TODO: proper allocator (caller frees this right now)
    struct OpenGLRenderer *ret = malloc(sizeof(struct OpenGLRenderer));

    GLint maxUniBindings;
    glGetIntegerv(GL_MAX_UNIFORM_BUFFER_BINDINGS, &maxUniBindings);
    if(UNIFORM_BUFFER_COUNT > maxUniBindings)
    {
        fprintf(stderr, "OpenGL: max uniform buffer bindings supported %d, but need at least %d\n", maxUniBindings, UNIFORM_BUFFER_COUNT);
        exit(-1);
    }

    GLint ibufsize;
    glGetIntegerv(GL_MAX_UNIFORM_BLOCK_SIZE, &ibufsize);
    ret->uniBufSize = ibufsize;

    glGenBuffers(INSTANCE_BUFFER_COUNT, ret->instanceDataBuffers);
    for(int i = 0; i < INSTANCE_BUFFER_COUNT; i++)
    {
        glBindBuffer(GL_UNIFORM_BUFFER, ret->instanceDataBuffers[i]);
        glBufferData(GL_UNIFORM_BUFFER, ret->uniBufSize, 0, GL_DYNAMIC_DRAW);
        glBindBufferBase(GL_UNIFORM_BUFFER, UNIFORM_BUFFER_INSTANCEDATA+i, ret->instanceDataBuffers[i]);
    }
    ret->curInstanceBufferIdx = 0;

    glGenBuffers(MATRIX_BUFFER_COUNT, ret->matrixBuffers);
    for(int i = 0; i < MATRIX_BUFFER_COUNT; i++)
    {
        glBindBuffer(GL_UNIFORM_BUFFER, ret->matrixBuffers[i]);
        glBufferData(GL_UNIFORM_BUFFER, ret->uniBufSize, 0, GL_DYNAMIC_DRAW);
        glBindBufferBase(GL_UNIFORM_BUFFER, UNIFORM_BUFFER_MATRICES+i, ret->matrixBuffers[i]);
    }
    ret->curMatrixBufferIdx = 0;

    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);

    return (struct Renderer*)ret;
}

void destroy_opengl_renderer(struct OpenGLRenderer *glrend)
{
    glDeleteBuffers(INSTANCE_BUFFER_COUNT, glrend->instanceDataBuffers);
    glDeleteBuffers(MATRIX_BUFFER_COUNT, glrend->matrixBuffers);
}

