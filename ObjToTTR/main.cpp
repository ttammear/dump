#if 0
set -e
clang++ -std=c++11 -ggdb3 ./main.cpp -o ./obj2ttr.out
cd ./sponza
../obj2ttr.out
exit
#endif

typedef struct V4
{
    float x, y, z, w;
} V4;

typedef struct V3
{
    float x, y, z;
} V3;

typedef struct V2
{
    float x, y;
} V2;

typedef struct Quat
{
    float w, x, y, z;
} Quat;

enum VertexAttributeType
{
    Vertex_Attribute_Type_None = 0,
    Vertex_Attribute_Type_Vec4,
    Vertex_Attribute_Type_Vec3,
    Vertex_Attribute_Type_Vec2,
    Vertex_Attribute_Type_Float
};

enum ShaderType
{
    Shader_Type_None,
    Shader_Type_Unlit_Color, // Color
    Shader_Type_Unlit_Vertex_Color, // nothing
    Shader_Type_Unlit_Textured, // texture, tint?
    Shader_Type_Unlit_Fade, // texture RGB and A, tint
    Shader_Type_Unlit_Textured_Cutout,
    Shader_Type_Gizmo, // Color
    Shader_Type_Count,
};

typedef struct Vert
{
    V3 position;
    V3 normal;
    V2 texCoord;
} Vert;

#define ALIGN_DOWN(n, a) ((n) & ~((a) - 1))
#define ALIGN_UP(n, a) ALIGN_DOWN((n) + (a) - 1, (a))
#define ALIGN_DOWN_PTR(p, a) ((void *)ALIGN_DOWN((uintptr_t)(p), (a)))
#define ALIGN_UP_PTR(p, a) ((void *)ALIGN_UP((uintptr_t)(p), (a)))

#define TINYOBJLOADER_IMPLEMENTATION
#include "libs/tinyobjloader/tiny_obj_loader.h"
#define STB_IMAGE_IMPLEMENTATION
#include "libs/stb_image.h"
#include <cstdint>
#include "/keep/Projects/ExpEngine/resourceformat.c"

#include <cstdio>
#include <iostream>
#include <set>
#include <cstring>
#include <algorithm>
#include <unordered_map>

struct VertIndex
{
    int posIdx;
    int normIdx;
    int texCoordIdx;
    int idx;
};

struct v_compare {
    bool operator() (const VertIndex& lhs, const VertIndex& rhs) const {
        return lhs.posIdx < rhs.posIdx 
            || (lhs.posIdx == rhs.posIdx && lhs.normIdx < rhs.normIdx) 
            || (lhs.posIdx == rhs.posIdx && lhs.normIdx == rhs.normIdx && lhs.texCoordIdx < rhs.texCoordIdx);
    }
};

void write_mesh(std::vector<Vert> &vertices, std::vector<int> &indices, const char *name, V3 offset)
{
    void *buf = malloc(10*1024*1024);
    struct TTRHeader *header = (struct TTRHeader*)buf;
    header->signature = TTR_4CHAR("TTR ");
    uint8_t *stream = (uint8_t*)buf + sizeof(struct TTRHeader);

    struct TTRDescTbl *descTbl = STREAM_PUSH_FLEX(stream, struct TTRDescTbl, entries, 1);
    descTbl->entryCount = 1;

    assert(sizeof(Vert) == 32);

    uint32_t vertSize = 52;

    uint32_t vertBufSize = vertices.size()*vertSize;
    struct TTRBuffer *vbuf = STREAM_PUSH_FLEX(stream, struct TTRBuffer, data, vertBufSize);
    vbuf->size = vertBufSize;
    uint8_t *vertStream = vbuf->data;
    for(int i = 0; i < vertices.size(); i++)
    {
        struct V4* pos = STREAM_PUSH(vertStream, struct V4);
        struct V2* texC = STREAM_PUSH(vertStream, struct V2);
        struct V4* col = STREAM_PUSH(vertStream, struct V4);
        struct V3* norm = STREAM_PUSH(vertStream, struct V3);
        V3 pos3 = vertices[i].position;
        V3 norm3 = vertices[i].normal;
        *pos = (V4){pos3.x-offset.x, pos3.y-offset.y, pos3.z-offset.z, 1.0f};
        //*col = (V4){norm3.x/2.0f+0.5f, norm3.y/2.0f+0.5f, norm3.z/2.0f+0.5f, 1.0f};
        *col = (V4){1.0f, 1.0f, 1.0f, 1.0f};
        *texC = vertices[i].texCoord;
        *norm = vertices[i].normal;
    }

    uint32_t indexBufSize = indices.size()*2;
    struct TTRBuffer *ibuf = STREAM_PUSH_FLEX(stream, struct TTRBuffer, data, indexBufSize);
    ibuf->size = indexBufSize;
    uint16_t *indexStream = (uint16_t*)ibuf->data;
    assert(indices.size() % 3 == 0);
    for(int i = 0; i < indices.size(); i+=3)
    {
        indexStream[i] = indices[i];
        indexStream[i+1] = indices[i+2]; // change winding order
        indexStream[i+2] = indices[i+1];
    }

    struct TTRMeshDesc *mdesc = STREAM_PUSH_FLEX(stream, struct TTRMeshDesc, attrs, 3);
    mdesc->indexSize = 2;
    mdesc->vertStride = vertSize;
    mdesc->numAttrs = 3;

    mdesc->attrs[0] = Vertex_Attribute_Type_Vec2; // texcoord
    mdesc->attrs[1] = Vertex_Attribute_Type_Vec4; // color
    mdesc->attrs[2] = Vertex_Attribute_Type_Vec3; // normal

    struct TTRMesh* mesh = STREAM_PUSH_FLEX(stream, struct TTRMesh, sections, 1);
    TTR_SET_REF_TO_PTR(mesh->descRef, mdesc);
    TTR_SET_REF_TO_PTR(mesh->vertBufRef, vbuf);
    TTR_SET_REF_TO_PTR(mesh->indexBufRef, ibuf);
    mesh->numVertices = vertices.size();
    mesh->numIndices = indices.size();
    mesh->numSections = 1;
    mesh->sections[0].startIndex = 0;
    mesh->sections[0].indexCount = indices.size();
//    printf("%p %p %p %p\n", buf, mesh, mesh, mesh3);

    TTR_SET_REF_TO_PTR(header->descTblRef, descTbl);
    TTR_SET_REF_TO_PTR(descTbl->entries[0].ref, mesh);
    descTbl->entries[0].type = TTR_4CHAR("MESH");
    strncpy(descTbl->entries[0].assetName, name, TTR_MAX_NAME_LEN);

    char meshpath[1024];
    strcpy(meshpath, "/keep/Projects/ExpEngineBuild/Packages/Sponza/");
    strcat(meshpath, name);
    strcat(meshpath, ".ttr"); 
    FILE *file = fopen(meshpath, "w+b");
    if(file)
    {
        uint32_t size = stream - (uint8_t*)buf;
        fwrite(buf, 1, size, file);
        fclose(file);
    }
    else
        fprintf(stderr, "Writing %s failed!\n", name);
    free(buf);

}

void write_objects(std::vector<std::string> &meshnames, std::vector<int> shapeMaterials, std::vector<tinyobj::material_t> materials, std::vector<std::string> textureAssetNames)
{
    void *buf = malloc(1024*1024);
    struct TTRHeader *header = (struct TTRHeader*)buf;
    header->signature = TTR_4CHAR("TTR ");
    uint8_t *stream = (uint8_t*)buf + sizeof(struct TTRHeader);

    int textureCount = 0;
    // TODO: use a set!!! import table will have dublicate entries
    for(int i = 0; i < shapeMaterials.size(); i++)
    {
        if(textureAssetNames[shapeMaterials[i]].length() > 0)
            textureCount++;
    }
    printf("objects have %d textures\n", textureCount);

    struct TTRDescTbl *dtbl = STREAM_PUSH_FLEX(stream, struct TTRDescTbl, entries, meshnames.size());
    dtbl->entryCount = meshnames.size();

    uint32_t importCount = meshnames.size() + textureCount;
    struct TTRImportTbl *itbl = STREAM_PUSH_FLEX(stream, struct TTRImportTbl, entries, importCount);
    itbl->entryCount = importCount;

    int texRef = meshnames.size();
    for(int i = 0; i < meshnames.size(); i++)
    {
        struct TTRObject *obj = STREAM_PUSH(stream, struct TTRObject);
        struct TTRMaterial *mat = STREAM_PUSH(stream, struct TTRMaterial);
        TTR_SET_REF_TO_PTR(obj->materialRef, mat);

        if(textureAssetNames[shapeMaterials[i]].length() == 0)
        {
            mat->shaderType = Shader_Type_Unlit_Vertex_Color;
        }
        else if(materials[shapeMaterials[i]].alpha_texname.empty())
        {
            mat->shaderType = Shader_Type_Unlit_Textured;
            uint32_t curRef = texRef++;
            mat->albedoTexARef.tblIndex = curRef | TTR_AREF_EXTERN_MASK;
            strncpy(itbl->entries[curRef].packageName, "Sponza", TTR_MAX_NAME_LEN);
            strncpy(itbl->entries[curRef].assetName, textureAssetNames[shapeMaterials[i]].c_str(), TTR_MAX_NAME_LEN);
            //printf("mesh %s mat %s\n", meshnames[i].c_str(), textureAssetNames[shapeMaterials[i]].c_str());
        } else {
            mat->shaderType = Shader_Type_Unlit_Textured_Cutout;
            uint32_t curRef = texRef++;
            mat->albedoTexARef.tblIndex = curRef | TTR_AREF_EXTERN_MASK;
            strncpy(itbl->entries[curRef].packageName, "Sponza", TTR_MAX_NAME_LEN);
            strncpy(itbl->entries[curRef].assetName, textureAssetNames[shapeMaterials[i]].c_str(), TTR_MAX_NAME_LEN);
        }
        mat->tintColor = {1.0f, 1.0f, 1.0f, 1.0f};

        obj->meshARef.tblIndex = i | TTR_AREF_EXTERN_MASK;
        dtbl->entries[i].type = TTR_4CHAR("OBJ ");
        TTR_SET_REF_TO_PTR(dtbl->entries[i].ref, obj);
        char strBuf[64];
        strcpy(strBuf, meshnames[i].c_str());
        strcat(strBuf, "_o");
        strncpy(dtbl->entries[i].assetName, strBuf, TTR_MAX_NAME_LEN);
        printf("object %s\n", strBuf);

        itbl->entries[i].type = TTR_4CHAR("MESH");
        strncpy(itbl->entries[i].assetName, meshnames[i].c_str(), TTR_MAX_NAME_LEN);
        strncpy(itbl->entries[i].packageName, "Sponza\0", TTR_MAX_NAME_LEN);
    }

    header->majorVersion = 0;
    header->minorVersion = 1;
    TTR_SET_REF_TO_PTR(header->descTblRef, dtbl);
    TTR_SET_REF_TO_PTR(header->importTblRef, itbl);

    char filePath[1024];
    strcpy(filePath, "/keep/Projects/ExpEngineBuild/Packages/Sponza/objects.ttr");
    FILE *file = fopen(filePath, "w+b");
    if(file)
    {
        uint32_t size = stream - (uint8_t*)buf;
        fwrite(buf, 1, size, file);
        fclose(file);
    }
    else
        fprintf(stderr, "Writing object file failed!\n");

    free(buf);

}

void write_texture(const char *filename, const char *cutoutFilename, const char *assetname)
{
    int w, h, comp;
    int cw, ch, ccomp;

    void *data = stbi_load(filename, &w, &h, &comp, 4);
    uint8_t *cutoutdata = NULL;
    if(cutoutFilename != NULL) {
        printf("Files %s and %s\n", filename, cutoutFilename);
        cutoutdata = (uint8_t*)stbi_load(cutoutFilename, &cw, &ch, &ccomp, 1);
        assert(cutoutdata); // failed to load cutout texture
        assert(w == cw && h == ch);
    }

    if(data == NULL)
    {
        fprintf(stderr, "Failed to load image %s, reason: %s\n", filename, stbi_failure_reason());
        return;
    }

    void *buf = malloc(4 * w * h + 1024*1024); // TODO: probably don't need an entire MB extra?
    TTRHeader *header = (struct TTRHeader*)buf;
    header->signature = TTR_4CHAR("TTR ");
    uint8_t *stream = (uint8_t*)buf + sizeof(TTRHeader);
    TTRDescTbl *dtbl = STREAM_PUSH_FLEX(stream, TTRDescTbl, entries, 1);
    dtbl->entryCount = 1;
    TTRImportTbl *itbl = STREAM_PUSH_FLEX(stream, TTRImportTbl, entries, 0);
    itbl->entryCount = 0;

    TTRTexture *ttex = STREAM_PUSH(stream, TTRTexture);
    ttex->format = 1;
    ttex->width = w;
    ttex->height = h;

    TTRBuffer *tbuf = STREAM_PUSH_FLEX(stream, TTRBuffer, data, w*h*4);
    tbuf->size = w*h*4;
    memcpy(tbuf->data, data, w*h*4);
    // set alpha channel if there is cutout texture
    if(cutoutdata != NULL) {
        for(int i = 0; i < w*h; i++) {
            tbuf->data[i*4+3] = cutoutdata[i];
        }
    }
    TTR_SET_REF_TO_PTR(ttex->bufRef, tbuf);

    header->majorVersion = 0;
    header->minorVersion = 1;
    TTR_SET_REF_TO_PTR(header->descTblRef, dtbl);
    TTR_SET_REF_TO_PTR(header->importTblRef, itbl);

    dtbl->entries[0].type = TTR_4CHAR("TEX ");
    strcpy(dtbl->entries[0].assetName, assetname);
    TTR_SET_REF_TO_PTR(dtbl->entries[0].ref, ttex);

    char outfileBuf[1024];
    strcpy(outfileBuf, "/keep/Projects/ExpEngineBuild/Packages/Sponza/");
    strcat(outfileBuf, assetname);
    strcat(outfileBuf, ".ttr");

    FILE *file = fopen(outfileBuf, "w+b");
    if(file)
    {
        uint32_t size = stream - (uint8_t*)buf;
        fwrite(buf, 1, size, file);
        fclose(file);
    }
    else
        fprintf(stderr, "Writing texture file %s failed!\n", outfileBuf);

    free(buf);
    stbi_image_free(data);
    if(cutoutdata != NULL) {
        stbi_image_free(cutoutdata);
    }
}

void write_textures(std::vector<tinyobj::material_t> &materials, std::vector<std::string> &textureAssetNames)
{
    for(auto it = materials.begin(); it != materials.end(); it++)
    {
        std::string diffuseTex = (*it).ambient_texname;
        std::string cutoutTex = (*it).alpha_texname;
        printf("material ambient: %s diffuse: %s specular: %s specular_highlight: %s bump: %s displacement: %s alpha: %s reflection: %s\n", (*it).ambient_texname.c_str(), (*it).diffuse_texname.c_str(), (*it).specular_texname.c_str(), (*it).specular_highlight_texname.c_str(), (*it).bump_texname.c_str(), (*it).displacement_texname.c_str(), (*it).alpha_texname.c_str(), (*it).reflection_texname.c_str());
        if(diffuseTex.length() > 0)
        {
            char assetName[1024];
            std::replace(diffuseTex.begin(), diffuseTex.end(), '\\', '/');
            std::replace(cutoutTex.begin(), cutoutTex.end(), '\\', '/');
            char *extension = strrchr(diffuseTex.c_str(), '.');

            const char *fname = strrchr(diffuseTex.c_str(), '/') + 1;
            fname = fname == NULL ? diffuseTex.c_str() : fname;

            if(extension != fname && strlen(fname) != 0)
            {
                uint32_t len = extension - fname;
                memcpy(assetName, fname, len);
                assetName[len] = 0;
                if(cutoutTex.empty()) {
                    write_texture(diffuseTex.c_str(), NULL, assetName);
                } else {
                    write_texture(diffuseTex.c_str(), cutoutTex.c_str(), assetName);
                }
                printf("add texture %s extension %s asset %s\n", diffuseTex.c_str(), extension, assetName);
                textureAssetNames.push_back(assetName);
            }
            else
            {
                fprintf(stderr, "Failed to gen name for texture %s\n", diffuseTex.c_str());
                textureAssetNames.push_back("");
            }
        }
        else
            textureAssetNames.push_back("");

    }
}

void write_map(std::vector<std::string> &meshnames, std::vector<V3> &positions) {
    void *mem = malloc(1024 * 1024);

    //uint8_t *stream = (uint8_t*)mem;
    TTRHeader *header = (struct TTRHeader*)mem;
    header->signature = TTR_4CHAR("TTR ");
    uint8_t *stream = (uint8_t*)mem + sizeof(TTRHeader);
    STREAM_PUSH_ALIGN(stream, 8);
    TTRDescTbl *dtbl = STREAM_PUSH_FLEX(stream, TTRDescTbl, entries, 1);
    dtbl->entryCount = 1;
    STREAM_PUSH_ALIGN(stream, 8);
    TTRImportTbl *itbl = STREAM_PUSH_FLEX(stream, TTRImportTbl, entries, 0);
    itbl->entryCount = 0;

    TTRMap *tmap = STREAM_PUSH(stream, TTRMap);
    STREAM_PUSH_ALIGN(stream, 8);
    TTRMapObjectTable *tmot = STREAM_PUSH_FLEX(stream, TTRMapObjectTable, entries, meshnames.size());
    STREAM_PUSH_ALIGN(stream, 8);
    TTRMapEntityTable *tmet = STREAM_PUSH_FLEX(stream, TTRMapEntityTable, entries, meshnames.size());

    tmot->numEntries = meshnames.size();
    tmet->numEntries = meshnames.size();

    for(int i = 0; i < meshnames.size(); i++) {
        char objname[1024];
        strcpy(objname, "Sponza/");
        strcat(objname, meshnames[i].c_str());
        strcat(objname, "_o");
        strcpy(tmot->entries[i].assetId, objname);
        tmot->entries[i].objectId = i+1;
        tmet->entries[i].position = positions[i];
        tmet->entries[i].scale = (V3){1.0f, 1.0f, 1.0f};
        tmet->entries[i].rotation = (Quat){1.0f, 0.0f, 0.0f, 0.0f};
        tmet->entries[i].objectId = i+1;
    }

    TTR_SET_REF_TO_PTR(tmap->objectTableRef, tmot);
    TTR_SET_REF_TO_PTR(tmap->entityTableRef, tmet);

    header->majorVersion = 0;
    header->minorVersion = 1;
    TTR_SET_REF_TO_PTR(header->descTblRef, dtbl);
    TTR_SET_REF_TO_PTR(header->importTblRef, itbl);

    dtbl->entries[0].type = TTR_4CHAR("MAP ");
    strcpy(dtbl->entries[0].assetName, "Sponza");
    TTR_SET_REF_TO_PTR(dtbl->entries[0].ref, tmap);
    
    FILE *file = fopen("/keep/Projects/ExpEngineBuild/Packages/Sponza/sponza.ttr", "w+b");
    if(file) {
        uint32_t size = stream - (uint8_t*)mem;
        fwrite(mem, 1, size, file);
        fclose(file);
        printf("Wrote .ttr map file (%dB) with %d objects and %d entities\n", size, tmot->numEntries, tmet->numEntries);
    } else
        fprintf(stderr, "Writing map file failed!\n");

    free(mem);
}

int main(int argc, const char *argv[])
{
    std::string inputfile = "sponza.obj";
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
      
    std::string err;
    bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &err, inputfile.c_str());
      
    if (!err.empty()) { // `err` may contain warning message.
      std::cerr << err << std::endl;
    }

    if (!ret) {
      exit(1);
    }

    float scale = 0.1f;

    typedef struct MeshData {
        std::string name;
        std::set<VertIndex, v_compare> indexSet;
        std::vector<Vert> vertices;
        std::vector<int> indices;
    } meshData;
    
    //std::set<VertIndex, v_compare> indexSet;
    //std::vector<Vert> vertices;
    //std::vector<int> indices;

    std::vector<std::string> names;
    std::vector<V3> positions;
    std::vector<int> shapeMats;

    std::unordered_map<int, MeshData*> meshMap; // key is materialId, we have to separate shapes with different material to different (sub)meshes

    // Loop over shapes
    //  Loop over faces (polygons)
    //  Loop over face vertices
    //      If vertex is unique push the vertex and its index
    //      Otherwise push the existing index
    //      WE ASSUME THE FACE HAS 3 vertices! (4+vertex polygons are not supported)
    //
    //  To handle materials correctly we need to create new mesh for every materialid of the shape

    // Loop over shapes
    for (size_t s = 0; s < shapes.size(); s++) {

      //indexSet.clear();
      //vertices.clear();
      //indices.clear();

      meshMap.clear();

      V3 average = {0};
      uint32_t acount = 0;

      // Loop over faces(polygon)
      size_t index_offset = 0;
      for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++) {
        int fv = shapes[s].mesh.num_face_vertices[f];
        int materialId = shapes[s].mesh.material_ids[f];

        auto m = meshMap.find(materialId);
        MeshData *smesh = NULL;
        if(m != meshMap.end()) {
            smesh = m->second;
        } else {
            smesh = new MeshData();
            auto result = meshMap.emplace(materialId, smesh);
            assert(result.second);
        }

        // Loop over vertices in the face.
        for (size_t v = 0; v < fv; v++) {
          // access to vertex
          tinyobj::index_t idx = shapes[s].mesh.indices[index_offset + v];

          int curidx = smesh->vertices.size();
          VertIndex vidx = {idx.vertex_index, idx.normal_index, idx.texcoord_index, curidx};
          auto result = smesh->indexSet.insert(vidx);
          if(result.second == true) // new element
          {
                tinyobj::real_t *p = &attrib.vertices[3*vidx.posIdx+0];
                V3 pos = {p[0]*scale, p[1]*scale, p[2]*scale};
                tinyobj::real_t *n = &attrib.normals[3*vidx.normIdx+0];
                V3 norm = {n[0], n[1], n[2]};
                tinyobj::real_t *t = &attrib.texcoords[2*vidx.texCoordIdx+0];
                V2 tex = {t[0], t[1]};
                smesh->vertices.push_back((Vert){pos, norm, tex});
                smesh->indices.push_back(curidx);
                average.x+=pos.x;average.y+=pos.y;average.z+=pos.z;acount++;
          }
          else // existing vertex
          {
              smesh->indices.push_back((*result.first).idx);
          }
          // Optional: vertex colors
          // tinyobj::real_t red = attrib.colors[3*idx.vertex_index+0];
          // tinyobj::real_t green = attrib.colors[3*idx.vertex_index+1];
          // tinyobj::real_t blue = attrib.colors[3*idx.vertex_index+2];
        }
        assert(fv == 3 || fv == 0);
        index_offset += fv;

        // per-face material
        //shapes[s].mesh.material_ids[f];
      }

      average.x /= (float)acount; average.y /= (float)acount; average.z /= (float)acount;
      for(auto it = meshMap.begin(); it != meshMap.end(); it++) {
          MeshData *tmesh = it->second;
          int materialId = it->first;
          std::string name = (shapes[s].name + "_" + std::to_string(materialId));
          printf("shape %s %zu vertices %zu indices, material %d\n", name.c_str(), tmesh->vertices.size(), tmesh->indices.size(), materialId);
          write_mesh(tmesh->vertices, tmesh->indices, name.c_str(), average);
          names.push_back(name);
          positions.push_back(average);
          shapeMats.push_back(materialId);
      }

      /*average.x/=(float)acount;average.y/=(float)acount;average.z/=(float)acount;
      printf("shape %s %zu vertices %zu indices, material %d\n", shapes[s].name.c_str(), vertices.size(), indices.size(), shapes[s].mesh.material_ids[0]);
      write_mesh(vertices, indices, shapes[s].name.c_str(), average);
      names.push_back(shapes[s].name);
      positions.push_back(average);
      // TODO: need to split into separate meshes based on per face material
      shapeMats.push_back(shapes[s].mesh.material_ids[0]);*/
    }
    std::vector<std::string> textureAssetNames;
    write_textures(materials, textureAssetNames);
    write_objects(names, shapeMats, materials, textureAssetNames);
    write_map(names, positions);

    printf("shapes: %zu vertices: %zu normals: %zu texcoords: %zu\n", shapes.size(), attrib.vertices.size(), attrib.normals.size(), attrib.texcoords.size());

    return 0;
}

