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

u32 mesh_vert_buf_offsets(u32 flags, uint32_t numVerts, u32 *texOfst, u32 *normOfst, u32 *colOfst)
{
    unsigned long long rtdsc = __rdtsc();
    u32 ret = 0;
    if(BIT_IS_SET(flags, MeshFlag_HasVertices))
        ret += numVerts * sizeof(struct V3);
    *texOfst = ret;
    if(BIT_IS_SET(flags, MeshFlag_HasTexcoords))
        ret += numVerts * sizeof(struct V3);
    *normOfst = ret;
    if(BIT_IS_SET(flags, MeshFlag_HasNormals))
        ret += numVerts * sizeof(struct V3);
    *colOfst = ret;
    if(BIT_IS_SET(flags, MeshFlag_HasColors))
        ret += numVerts * sizeof(struct V4);
    return ret;
};

void opengl_update_mesh(struct Mesh *mesh)
{
    u32 mflags = mesh->flags;
    u32 nverts = mesh->numVertices;
    if(!BIT_IS_SET(mflags, MeshFlag_Initialized))
    {
        // TODO: create buffer and stuff
        u32 vbo, ebo, vao;
        glGenBuffers(1, &vbo);
        glGenBuffers(1, &ebo);
        glGenVertexArrays(1, &vao);

        u32 texOfst, normOfst, colOfst;
        u32 size = mesh_vert_buf_offsets(mflags, nverts, &texOfst, &normOfst, &colOfst);

        glBindVertexArray(vao);

        if(BIT_IS_SET(mflags, MeshFlag_HasVertices))
        {
            glBindBuffer(GL_ARRAY_BUFFER, vbo);
            glBufferData(GL_ARRAY_BUFFER, size, 0, GL_STATIC_DRAW);
            glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(struct V3) * nverts, mesh->vertices);
            glVertexAttribPointer(POSITION_ATTRIBUTE_LOCATION, 3, GL_FLOAT, false, 0, 0);
            glEnableVertexAttribArray(POSITION_ATTRIBUTE_LOCATION);
            if(BIT_IS_SET(mflags, MeshFlag_HasTexcoords))
            {
                glBufferSubData(GL_ARRAY_BUFFER, texOfst, sizeof(struct V3) *nverts, mesh->texCoords);
                glEnableVertexAttribArray(TEXCOORD_ATTRIBUTE_LOCATION);
                glVertexAttribPointer(TEXCOORD_ATTRIBUTE_LOCATION, 3, GL_FLOAT, false, 0, (GLvoid*)(uintptr_t)texOfst);
            }
            else
                glDisableVertexAttribArray(TEXCOORD_ATTRIBUTE_LOCATION);
            if(BIT_IS_SET(mflags, MeshFlag_HasNormals))
            {
                glBufferSubData(GL_ARRAY_BUFFER, normOfst, sizeof(struct V3) * nverts, mesh->normals);
                glEnableVertexAttribArray(NORMAL_ATTRIBUTE_LOCATION);
                glVertexAttribPointer(NORMAL_ATTRIBUTE_LOCATION, 3, GL_FLOAT, false, 0, (GLvoid*)(uintptr_t)normOfst);
            }
            else
                glDisableVertexAttribArray(NORMAL_ATTRIBUTE_LOCATION);
            if(BIT_IS_SET(mflags, MeshFlag_HasColors))
            {
                glBufferSubData(GL_ARRAY_BUFFER, colOfst, sizeof(struct V4) * nverts, mesh->colors);
                glEnableVertexAttribArray(COLOR_ATTRIBUTE_LOCATION);
                glVertexAttribPointer(COLOR_ATTRIBUTE_LOCATION, 4, GL_FLOAT, false, 0, (GLvoid*)(uintptr_t)colOfst);
            }
            else 
                glDisableVertexAttribArray(COLOR_ATTRIBUTE_LOCATION);
        }
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(mesh->indices[0]) * mesh->numIndices, mesh->indices, GL_STATIC_DRAW);
        mesh->rendererHandle = vbo;
        mesh->rendererHandle2 = ebo;
        mesh->rendererHandle3 = vao;
        SET_BIT(mesh->flags, MeshFlag_Initialized);
        CLEAR_BIT(mesh->flags, MeshFlag_Dirty);
        glBindVertexArray(0);
    }
    else if(BIT_IS_SET(mflags, MeshFlag_Dirty))
    {
        // TODO: when mesh is initialized, but dirty, update it
        printf("MESH DIRTYYY\n");
        assert(false);
    }
}

u32 tex_format_to_opengl_format(u16 format)
{
    switch(format)
    {
        case TextureFormat_RGB:
            return GL_RGB;
        case TextureFormat_RGBA:
            return GL_RGBA;
        default:
            // TODO: deal with None and R8
            assert(false);
            break;
    };
    return (u32)-1;
}

void opengl_update_texture(struct Texture *tex)
{
    printf("Opengl tex 0\n");
    u32 tflags = tex->flags;
    if(!BIT_IS_SET(tflags, TextureFlag_Initialized))
    {
        GLuint texH;
        glGenTextures(1, &texH);
        if(BIT_IS_SET(tflags, TextureFlag_HasData))
        {
            glBindTexture(GL_TEXTURE_2D, texH);
            u32 format = tex_format_to_opengl_format(tex->format);
            // TODO: if WEBGL 1, formats must match
            u32 size = tex->width*tex->height*tex_format_get_comps(tex->format);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tex->width, tex->height, 0, format, GL_UNSIGNED_BYTE, tex->data);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            printf("OpenGL new texture!\n");
        }
        glGenerateMipmap(GL_TEXTURE_2D);
        tex->rendererHandle = texH;
        SET_BIT(tex->flags, TextureFlag_Initialized);
    }
    else if(BIT_IS_SET(tflags, TextureFlag_Dirty))
    {
        // TODO: if already initialized then copy the new data in
        assert(false);
    }
};

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

bool opengl_is_valid_material(struct Material *material)
{
    bool ret = true;
    u32 dummy;
    ret = ret && material_find_shader_of_type(material, ShaderType_GLSL_Frag, &dummy);
    ret = ret && material_find_shader_of_type(material, ShaderType_GLSL_Vert, &dummy);
    return ret;
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

void opengl_update_material(struct Material *material)
{
    u32 mflags = material->flags;

    if(!BIT_IS_SET(mflags, MaterialFlag_Initialized))
    {
        GLuint program = glCreateProgram();
        material->rendererHandle = (uintptr_t)program;
        SET_BIT(material->flags, MaterialFlag_Initialized);
    }

    if(BIT_IS_SET(mflags, MaterialFlag_Dirty))
    {
        u32 numShadersToLink = 0;
        GLuint shadersToLink[MATERIAL_MAX_SHADERS];
        b32 needRelink = false;
        for(int i = 0; i < MATERIAL_MAX_SHADERS; i++)
        {
            struct Shader *shader = &material->shaders[i];
            GLuint glshader = 0;
            switch(shader->type)
            {
                case ShaderType_GLSL_Frag:
                case ShaderType_GLSL_Vert:
                    {
                        // REVIEW: i think this adds more complexity then it helps
                        b32 active = BIT_IS_SET(shader->flags, ShaderFlag_Active);
                        b32 initialized = BIT_IS_SET(shader->flags, ShaderFlag_Initialized);
                        b32 dirty = BIT_IS_SET(shader->flags, ShaderFlag_Dirty);
                        if(!initialized && active)
                        {
                            i32 len = strlen(shader->source);
                            GLenum gltype = shader_type_to_glenum(shader->type);
                            glshader = glCreateShader(gltype);
                            shader->rendererHandle = glshader;
                            // this will force (active && dirty) to load the shader
                            SET_BIT(shader->flags, ShaderFlag_Dirty);
                            dirty = true;
                            SET_BIT(shader->flags, ShaderFlag_Initialized);
                            initialized = true;
                        }
                        else if(initialized && !active)
                        {
                            glshader = (GLuint)shader->rendererHandle;
                            assert(glshader != 0);
                            glDeleteShader(glshader);
                            CLEAR_BIT(shader->flags, ShaderFlag_Initialized);
                            CLEAR_BIT(shader->flags, ShaderFlag_Dirty);
                            needRelink = true;
                            initialized = false;
                            dirty = false;
                        }
                        if(active && dirty)
                        {
                            assert(initialized);
                            glshader = (GLuint)shader->rendererHandle;
                            assert(glshader != 0);
                            volatile i32 len = strlen(shader->source);
                            assert(len > 0);
                            volatile const char *tempSource = shader->source;
                            glShaderSource(glshader, 1, (const GLchar*const*)&tempSource, (const GLint*)&len);
                            glCompileShader(glshader);
                            needRelink = true;
                            CLEAR_BIT(shader->flags, ShaderFlag_Dirty);
                            dirty = false;
                        }
                        if(active)
                        {
                            shadersToLink[numShadersToLink++] = (GLuint)shader->rendererHandle;
                            assert(numShadersToLink < ARRAY_COUNT(shadersToLink));
                            assert(shader->rendererHandle != 0);
                        }
                    } break;
                default:
                    break;
            }
        }
        if(needRelink)
        {
            GLuint program = (GLuint)material->rendererHandle;
            assert(glIsProgram(program));
            for(int i = 0; i < numShadersToLink; i++)
            {
                assert(glIsShader(shadersToLink[i]));
                glAttachShader(program, shadersToLink[i]); 
            }

            // bind engine attributes to a well known location
            glBindAttribLocation(program, POSITION_ATTRIBUTE_LOCATION, "coordinates");
            glBindAttribLocation(program, TEXCOORD_ATTRIBUTE_LOCATION, "a_texcoord");
            glBindAttribLocation(program, NORMAL_ATTRIBUTE_LOCATION, "a_normal");
            glBindAttribLocation(program, COLOR_ATTRIBUTE_LOCATION, "a_color");
            glBindAttribLocation(program, TANGENT_ATTRIBUTE_LOCATION, "a_tangent");

            glLinkProgram(program);
            // @Unoptimized: attaching and detaching on every link
            for(int i = 0; i < numShadersToLink; i++)
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
                SET_BIT(material->flags, MaterialFlag_Valid);
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
                CLEAR_BIT(material->flags, MaterialFlag_Valid);
            }
        }
        CLEAR_BIT(material->flags, MaterialFlag_Dirty);
    }
}

void opengl_destroy_material(struct Material *mat)
{
    // delete all shaders
    for(int i = 0; i < MATERIAL_MAX_SHADERS; i++)
    {
        struct Shader *shader = &mat->shaders[i];
        GLuint glshader = 0;
        switch(shader->type)
        {
            case ShaderType_GLSL_Frag:
            case ShaderType_GLSL_Vert:
                {
                    if(BIT_IS_SET(shader->flags, ShaderFlag_Initialized))
                    {
                        // not sure it makes any difference if the mesh is
                        // active or not, but I believe Active and Initialized
                        // should always be synchronized at this point
                        assert(BIT_IS_SET(shader->flags, ShaderFlag_Active));
                        glshader = (GLuint)shader->rendererHandle;
                        assert(glshader != 0);
                        glDeleteShader(glshader);
                        CLEAR_BIT(shader->flags, ShaderFlag_Initialized);
                        CLEAR_BIT(shader->flags, ShaderFlag_Dirty);
                    }
                } break;
            default:
                break;
        }
    }

    if(BIT_IS_SET(mat->flags, MaterialFlag_Initialized))
    {
        GLuint program = (GLuint)mat->rendererHandle;
        assert(program != 0 && program != GL_INVALID_INDEX);
        glDeleteProgram(program);
        CLEAR_BIT(mat->flags, MaterialFlag_Initialized);
        CLEAR_BIT(mat->flags, MaterialFlag_Valid);
        CLEAR_BIT(mat->flags, MaterialFlag_Dirty);
    }
}

void opengl_destroy_mesh(struct Mesh *mesh)
{
    u32 mflags = mesh->flags;
    if(BIT_IS_SET(mflags, MeshFlag_Initialized))
    {
        GLuint vbo = (GLuint)mesh->rendererHandle;
        GLuint ebo = (GLuint)mesh->rendererHandle2;
        GLuint vao = (GLuint)mesh->rendererHandle3;
        glDeleteBuffers(1, &vbo);
        glDeleteBuffers(1, &ebo);
        glDeleteVertexArrays(1, &vao);
        CLEAR_BIT(mesh->flags, MeshFlag_Initialized);
    }
}

void opengl_destroy_texture(struct Texture *tex)
{
    if(BIT_IS_SET(tex->flags, TextureFlag_Initialized))
    {
        GLuint gltex = (GLuint)tex->rendererHandle;
        glDeleteTextures(1, &gltex);
        CLEAR_BIT(tex->flags, TextureFlag_Initialized);
    }
}

void opengl_manage_render_event(u32 eventId, void *eventData, void* renderer)
{
    switch(eventId)
    {
        case RenderEvent_MeshUpdate:
            assert(eventData != NULL);
            opengl_update_mesh((struct Mesh*)eventData);
            break;
        case RenderEvent_MeshDestroy:
            printf("Mesh destroyed!!\n");
            assert(eventData != NULL);
            opengl_destroy_mesh((struct Mesh*)eventData);
            break;
        case RenderEvent_TextureUpdate:
            printf("Texture updatdddddd\n");
            assert(eventData != NULL);
            opengl_update_texture((struct Texture*)eventData);
            break; 
        case RenderEvent_TextureDestroy:
            assert(eventData != NULL);
            opengl_destroy_texture((struct Texture*)eventData);
            break;
        case RenderEvent_MaterialUpdate:
            {
                printf("OpenGL material update\n");
                bool valid = opengl_is_valid_material((struct Material*)eventData);
                if(valid)
                {
                    opengl_update_material((struct Material*)eventData);
                }
                else
                {
                    printf("Material updated, but material missing required shaders\n");
                }
                assert(eventData != NULL);
            } break;
        case RenderEvent_MaterialDestroy:
            opengl_destroy_material((struct Material*)eventData);
            break;
    }
}

static inline void opengl_change_instance_buffer(struct OpenGLRenderer *renderer)
{
    renderer->curInstanceBufferIdx = ++renderer->curInstanceBufferIdx%INSTANCE_BUFFER_COUNT;
}

void opengl_flush_instances(struct OpenGLRenderer *renderer, struct Mesh *mesh, struct Material *mat, u32 instanceCount, void *uniformData, u32 uniformDataSize, void *matData)
{
    PROF_START();
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
    u32 mflags = mesh->flags;
    if(BIT_IS_SET(mflags, MeshFlag_Dirty))
        opengl_update_mesh(mesh);

    glDrawElementsInstanced(GL_TRIANGLES, mesh->numIndices, GL_UNSIGNED_SHORT, 0, instanceCount);

    /*GLuint instIDU = glGetUniformLocation(program, "instId");
    for(u32 j = 0; j < instanceCount; j++)
    {
        glUniform1i(instIDU, j);
        glDrawElements(GL_TRIANGLES, mesh->numIndices, GL_UNSIGNED_SHORT, 0);
    }*/

    opengl_change_instance_buffer(renderer);
    PROF_END();
}

void opengl_render_view(struct OpenGLRenderer *renderer, struct RenderViewBuffer *rbuf)
{
    PROF_START();
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
        struct Material *material = mentry->material;

        // use fallback material if the one attached to mesh isn't valid
        if(!BIT_IS_SET(material->flags, MaterialFlag_Valid))
            material = &renderer->renderer.fallbackMaterial;

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
                opengl_flush_instances(renderer, mentry->mesh, material, instRunId, idbuf, idofst, mbuf);
                idofst = mofst = instRunId = 0;
            }

            memcpy(&idbuf[idofst], idptr, idsize);
            idofst += idsize;
            static_assert(sizeof(struct Mat4) == 4*4*4, "Mat4 assumed to be packed, otherwise it won't match OpenGL layout!");
            memcpy(&mbuf[mofst], &minstance->modelM, sizeof(struct Mat4));
            mofst += sizeof(struct Mat4);
        }
        // @OPTIMIZE: we currently reset buffer offset, but we could keep writing to
        // it until its full. the only thing we have to do is start writing from
        // where we were before
        if(idofst > 0 || mofst > 0)
        {
            opengl_flush_instances(renderer, mentry->mesh, material, instRunId, idbuf, idofst, mbuf);
            idofst = mofst = instRunId = 0;
        }
    }
    PROF_END();
}

struct Renderer *create_opengl_renderer()
{
    // TODO: proper allocator (caller frees this right now)
    struct OpenGLRenderer *ret = malloc(sizeof(struct OpenGLRenderer));
    ret->meshUpdateEventHandle = subscribe_to_render_event(RenderEvent_MeshUpdate, opengl_manage_render_event, ret);
    ret->meshDestroyEventHandle = subscribe_to_render_event(RenderEvent_MeshDestroy, opengl_manage_render_event, ret);
    ret->textureUpdateEventHandle = subscribe_to_render_event(RenderEvent_TextureUpdate, opengl_manage_render_event, ret);
    ret->textureDestroyEventHandle = subscribe_to_render_event(RenderEvent_TextureDestroy, opengl_manage_render_event, ret);
    ret->materialUpdateEventHandle = subscribe_to_render_event(RenderEvent_MaterialUpdate, opengl_manage_render_event, ret);
    ret->materialDestroyEventHandle = subscribe_to_render_event(RenderEvent_MaterialDestroy, opengl_manage_render_event, ret);

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

    unsubscribe_from_render_event(RenderEvent_MeshUpdate, glrend->meshUpdateEventHandle);
    unsubscribe_from_render_event(RenderEvent_MeshDestroy, glrend->meshDestroyEventHandle);
    unsubscribe_from_render_event(RenderEvent_TextureUpdate, glrend->textureUpdateEventHandle);
    unsubscribe_from_render_event(RenderEvent_TextureDestroy, glrend->textureDestroyEventHandle);
    unsubscribe_from_render_event(RenderEvent_MaterialUpdate, glrend->materialUpdateEventHandle);
    unsubscribe_from_render_event(RenderEvent_MaterialDestroy, glrend->materialDestroyEventHandle);
}

