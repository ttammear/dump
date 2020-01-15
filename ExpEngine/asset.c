


// Asset state machine
//
//  (wait)-----------------Pending Destroy <-----------------(enqueue)
//    |                                                             | 
//    -> None (enqueue)-> InQueue (wait)-> Loading (wait) -> Loaded--
//        |                     |                    |
//        ------------------(dequeue)                 -> Fail
//


// Asset actions are run when asset state changes ([newState][targetState])

TessAssetAction assetActionMatrix[6][6] = {
//                       None                   InQ                 L.ing          L.ed             Fail                P.D.             (target state)
/*None*/                {A_Noop,            A_Invalid_Target, A_Invalid_Target, A_Enqueue_Load,   A_Failure, A_Invalid_Target},
/*InQueue*/             {A_Cancel_Load,     A_Invalid_Target, A_Invalid_Target, A_Noop,           A_Failure, A_Invalid_Target},
/*Loading*/             {A_Noop,            A_Invalid_Target, A_Invalid_Target, A_Noop,           A_Failure, A_Invalid_Target},
/*Loaded*/              {A_Enqueue_Destroy, A_Invalid_Target, A_Invalid_Target, A_Noop,           A_Failure, A_Invalid_Target},
/*Fail*/                {A_Noop,            A_Invalid_Target, A_Invalid_Target, A_Noop,           A_Noop,    A_Invalid_Target},
/*Pending Destroy*/     {A_Noop,            A_Invalid_Target, A_Invalid_Target, A_Cancel_Destroy, A_Failure, A_Invalid_Target},
// (current state)
};

static_assert(Tess_Asset_Status_Count == sizeof(assetActionMatrix[0])/sizeof(__typeof(assetActionMatrix[0][0])), "If you add asset statuses, also add actions to the table!"); 


internal void tess_replace_id_name_characters(char buf[], const char *name, uint32_t buflen);
TStr* tess_asset_id_from_aref(TessAssetSystem *as, TTRDescTbl *tbl, TTRImportTbl *imTbl, TTRAssetRef aref, TStr *currentPackage);
void tess_fill_mesh_data(Renderer *renderer, MeshQueryResult *mqr, void *userData);
void tess_finalize_mesh(Renderer *renderer, MeshReady *mr, void *userData);
void tess_fill_texture_data(Renderer *renderer, TextureQueryResponse *tqr, void *userData);
void tess_finalize_texture(Renderer *renderer, TextureReady *tr, void *userData);
void tess_finalize_object(Renderer *renderer, MaterialReady *mr, void *userData);
internal void tess_async_wait_deps(struct TessLoadingAsset *lasset, AsyncTask *task);
TessAsset* tess_get_asset(TessAssetSystem *as, TStr *assetId);
TessAsset* tess_force_load_asset(TessAssetSystem *as, TStr *assetId); 
internal void loading_asset_init(TessAssetSystem *as, TessLoadingAsset *lasset, TStr *fileName, TStr *assetId);
internal void tess_loading_asset_done(TessAssetSystem *as, TessLoadingAsset *lasset, TessAsset *assett, TessAssetStatus newStatus, bool completed);
internal void asset_loaded(TessAssetSystem *as, TessAsset *asset);
ASYNC void tess_unlist_asset(TessAssetSystem *as, TessAsset *asset);
ASYNC void fill_material_query(RenderMessage *msg, TTRMaterial *ttrMat, TessTTRContext *ttrCtx, AssetReference **ref, TStr *parentId);

internal void tess_set_asset_status(TessAssetSystem *as, TStr *assetId, uint32_t status);
ASYNC void tess_task_unload_asset(void *data);
internal bool tess_queue_asset(TessAssetSystem *as, TStr *assetId);

internal void tess_asset_action(TessAssetSystem *as, TStr *assetId, TessAssetAction action) {
    TessLoadingAsset *lasset;
    TessAsset *asset;
    khiter_t k;
    switch(action) {
        case A_Noop:
            break;
        case A_Enqueue_Load:
            tess_queue_asset(as, assetId);
            break;
        case A_Enqueue_Destroy:
            asset = tess_get_asset(as, assetId);
            assert(asset); // why would we destroy asset that doesn't exist?
            tess_set_asset_status(as, assetId, Tess_Asset_Status_Pending_Destroy);
            asset->ul = (struct UnloadAssetData*)malloc(sizeof(struct UnloadAssetData));
            asset->ul->as = as;
            asset->ul->assetId = assetId;
            asset->ul->task = scheduler_queue_task(tess_task_unload_asset, (void*)asset->ul);
            break;
        case A_Cancel_Load:
            k = kh_get(64, as->loadingAssetMap, (intptr_t)assetId);
            assert(k != kh_end(as->loadingAssetMap));
            lasset = kh_value(as->loadingAssetMap, k);
            scheduler_cancel_task(lasset->task);
            //printf("cancel %p\r\n", lasset);
            tess_loading_asset_done(as, lasset, NULL, Tess_Asset_Status_None, false);
            break;
        case A_Cancel_Destroy:
            asset = tess_get_asset(as, assetId);
            scheduler_cancel_task(asset->ul->task);
            free(asset->ul);
            asset->ul = NULL;
            break;
        case A_Failure:
            printf("Loading asset failed %s\r\n", assetId->cstr);
            break;
        case A_Invalid_Target:
            assert(0); // intermediate status is not a valid status
            break;
        default:
            assert(0); // Unknown action
            break;
    }
}

internal void tess_set_asset_status(TessAssetSystem *as, TStr *assetId, uint32_t status)
{
    int dummy;
    khiter_t k;
    TessAssetAction a;
    TessAssetStatus target;
    if(status != Tess_Asset_Status_None) {
        k = kh_put(uint32, as->assetStatusMap, (intptr_t)assetId, &dummy);
        kh_value(as->assetStatusMap, k) = status;
    } else {
        k = kh_get(uint32, as->assetStatusMap, (intptr_t)assetId);
        if(k != kh_end(as->assetStatusMap)) {
            kh_del(uint32, as->assetStatusMap, k);
        }
    }
    target = tess_get_asset_target_status(as, assetId);
    a = assetActionMatrix[status][target];
    tess_asset_action(as, assetId, a);
}

internal void tess_set_asset_target_status(TessAssetSystem *as, TStr *assetId, uint32_t target) {
    int dummy;
    khiter_t k;
    TessAssetAction a;
    TessAssetStatus status;
    if(target != Tess_Asset_Status_None) {
        k = kh_put(uint32, as->assetTargetStatusMap, (intptr_t)assetId, &dummy);
        kh_value(as->assetTargetStatusMap, k) = target;
    } else { 
        k = kh_get(uint32, as->assetTargetStatusMap, (intptr_t)assetId);
        if(k != kh_end(as->assetTargetStatusMap)) {
            kh_del(uint32, as->assetTargetStatusMap, k);
        }
    }
    status = tess_get_asset_status(as, assetId);
    a = assetActionMatrix[status][target];
    tess_asset_action(as, assetId, a);
}

internal inline bool tess_is_asset_loading(TessAssetSystem *as, TStr *assetId)
{
    auto status = tess_get_asset_status(as, assetId);
    return status == Tess_Asset_Status_Loading;
}

void tess_get_asset_metrics(struct TessAssetSystem *as, struct TessAssetSystemMetrics *tasm) {
    tasm->numLoadingAssets = as->numLoadingAssets;
    tasm->numLoadedAssets = as->numLoadedAssets;
    tasm->numOpenedAssetFiles = as->numOpenAssetFiles;
    tasm->totalFileLoads = as->totalFileLoads;
}

uint32_t tess_get_asset_list(struct TessAssetSystem *as, TessListedAsset* listedAssets) {
    uint32_t len = kh_size(as->assetStatusMap);
    khiter_t k;
    uint32_t count = 0;
    if(listedAssets != NULL) {
        for(k = kh_begin(as->assetStatusMap); k != kh_end(as->assetStatusMap); ++k) {
            if(kh_exist(as->assetStatusMap, k)) {
                TStr *assetId = (TStr*)kh_key(as->assetStatusMap, k);
                TessAsset *asset = tess_get_asset(as, assetId);
                listedAssets[count].assetId = assetId;
                listedAssets[count].status = kh_value(as->assetStatusMap, k);
                listedAssets[count].target = tess_get_asset_target_status(as, assetId);
                if(asset != NULL) {
                    listedAssets[count].type = asset->type;
                } else {
                    listedAssets[count].type = Tess_Asset_None;
                }
                khiter_t refK = kh_get(ptrToU32, as->refCountMap, (intptr_t)assetId);
                if(refK != kh_end(as->refCountMap)) {
                    listedAssets[count].refCount = kh_value(as->refCountMap, refK);
                } else {
                    listedAssets[count].refCount = 0;
                }
                count++;
            }
        }
        assert(count == len);
    }
    return len;
}

void tess_finalize_asset(TessAssetSystem *as, TessLoadingAsset *lasset, TessAsset *asset, TStr *assetId, enum TessAssetType type) {
    asset->assetId = assetId;
    asset->type = type;
    //printf("Asset loaded! %s\n", lasset->assetId->cstr);
    tess_loading_asset_done(as, lasset, asset, Tess_Asset_Status_Loaded, true);
    as->numLoadedAssets++;
}

ASYNC TessTextureAsset* tess_load_texture(TessAssetSystem *as, struct TessLoadingAsset *lasset, TTRTexture *ttex) {
    scheduler_assert_task(); // this is an async function
    AsyncTask task;
    
    RenderMessage msg = {};
    msg.type = Render_Message_Texture_Query;
    msg.texQ.textureId = 0;
    msg.texQ.width = ttex->width;
    msg.texQ.height = ttex->height;
    msg.texQ.format = Texture_Format_RGBA;
    msg.texQ.filter = Texture_Filter_Trilinear;
    renderer_async_message(as->renderer, &task, &msg);
    scheduler_wait_for(&task);
    TTRBuffer *tbuf = TTR_REF_TO_PTR(TTRBuffer, ttex->bufRef);
    assert(tbuf->size == ttex->width*ttex->height*4);
    memcpy(task.renderMsg->texQR.textureDataPtr, tbuf->data, tbuf->size);
    msg = (const struct RenderMessage){0};
    msg.type = Render_Message_Texture_Update;
    msg.texU.textureId = task.renderMsg->texQR.textureId;
    renderer_async_message(as->renderer, &task, &msg);
    scheduler_wait_for(&task);

    TessTextureAsset *texture = pool_allocate(as->texturePool);
    assert(texture);
    texture->textureId = task.renderMsg->texR.textureId;

    tess_finalize_asset(as, lasset, &texture->asset, lasset->assetId, Tess_Asset_Texture);

    return texture;
}

ASYNC void tess_unload_texture(TessAssetSystem *as, TessTextureAsset *tex) {
    scheduler_assert_task(); // this is an async function 
    RenderMessage msg = {0};
    msg.type = Render_Message_Texture_Destroy;
    msg.texD.textureId = tex->textureId;
    tex->textureId = 0; // reduntant
    renderer_queue_message(as->renderer, &msg); // TODO: this should be async?
    
    tess_unlist_asset(as, &tex->asset);
    
    pool_free(as->texturePool, tex);
}

ASYNC TessMeshAsset* tess_load_mesh(TessTTRContext *tctx, TessLoadingAsset *lasset, TTRMesh *tmesh) {

    AssetReference *texRef;
    AsyncTask task;
    TTRMeshDesc *ttrMeshDesc = TTR_REF_TO_PTR(TTRMeshDesc, tmesh->descRef);
    RenderMessage msg = {};
    RenderMessage mmsg = {};
    msg.type = Render_Message_Mesh_Query;
    msg.meshQuery.meshId = 0;
    msg.meshQuery.vertexCount = tmesh->numVertices;
    msg.meshQuery.indexCount = tmesh->numIndices;
    for(int j = 0; j < ttrMeshDesc->numAttrs; j++) {
        msg.meshQuery.attributeTypes[j] = ttrMeshDesc->attrs[j];
    }
    renderer_async_message(tctx->as->renderer, &task, &msg);
    scheduler_wait_for(&task);
    TTRBuffer *vbuf = TTR_REF_TO_PTR(TTRBuffer, tmesh->vertBufRef);
    TTRBuffer *ibuf = TTR_REF_TO_PTR(TTRBuffer, tmesh->indexBufRef);
    // TODO: check if buffer sizes match
    memcpy(task.renderMsg->meshQueryResult.vertBufPtr, vbuf->data, vbuf->size);
    memcpy(task.renderMsg->meshQueryResult.idxBufPtr, ibuf->data, ibuf->size);

    TessMeshAsset *mesh = pool_allocate(tctx->as->meshPool);

    // update mesh
    msg = (const RenderMessage){};
    msg.type = Render_Message_Mesh_Update;
    msg.meshUpdate.meshId = task.renderMsg->meshQueryResult.meshId;
    assert(tmesh->numSections <= MAX_MESH_SECTIONS);
    msg.meshUpdate.numSections = tmesh->numSections;
    mesh->numSections = tmesh->numSections;

    for(int i = 0; i < tmesh->numSections; i++) {
        TTRMaterial *ttrMat = TTR_REF_TO_PTR(TTRMaterial, tmesh->sections[i].materialRef);
        TTRAssetRef aref;
        TStr *textureAssetId;
        TessAsset *texAsset;
        switch(ttrMat->shaderType) {
            case Shader_Type_Unlit_Textured_Cutout:
            case Shader_Type_Unlit_Textured:
                aref = ttrMat->albedoTexARef;
                textureAssetId = tess_asset_id_from_aref(tctx->as, tctx->tbl, tctx->imTbl, aref, tctx->package);
                break;
        }
    }

    for(int i = 0; i < tmesh->numSections; i++) {
        msg.meshUpdate.sections[i].offset = tmesh->sections[i].startIndex;
        msg.meshUpdate.sections[i].count = tmesh->sections[i].indexCount;

        TTRMaterial* ttrMat = TTR_REF_TO_PTR(TTRMaterial, tmesh->sections[i].materialRef);
        mmsg = (const RenderMessage){};
        fill_material_query(&mmsg, ttrMat, tctx, &texRef, lasset->assetId);
        renderer_async_message(tctx->as->renderer, &task, &mmsg);
        scheduler_wait_for(&task); // TODO: submit all then wait?
        msg.meshUpdate.sections[i].materialId = task.renderMsg->matR.materialId;
        mesh->materials[i].materialId = task.renderMsg->matR.materialId;
        mesh->materials[i].texRef = texRef;
    }

    renderer_async_message(tctx->as->renderer, &task, &msg);
    scheduler_wait_for(&task);
    assert(mesh);
    mesh->meshId = task.renderMsg->meshR.meshId;

    tess_finalize_asset(tctx->as, lasset, &mesh->asset, lasset->assetId, Tess_Asset_Mesh);

    return mesh;
}

ASYNC void tess_unload_mesh(TessAssetSystem *as, TessMeshAsset *mesh) {
    scheduler_assert_task(); // this is an async function
    RenderMessage msg = {0};
    msg.type = Render_Message_Mesh_Destroy;
    msg.meshD.meshId = mesh->meshId;
    mesh->meshId = 0; // reduntant
    renderer_queue_message(as->renderer, &msg); // TODO: this should be async?

    for(int i = 0; i < mesh->numSections; i++) {
        msg.type = Render_Message_Material_Destroy;
        msg.matD.materialId = mesh->materials[i].materialId;
        // TODO: decrement texture reference count, if material has one
        renderer_queue_message(as->renderer, &msg);
        if(mesh->materials[i].texRef != NULL) {
            remove_asset_reference(as, mesh->materials[i].texRef);
        }
        mesh->materials[i] = (TessMaterial){0}; // redundant
    }
    
    tess_unlist_asset(as, &mesh->asset);
    
    pool_free(as->meshPool, mesh);
}

ASYNC void fill_material_query(RenderMessage *msg, TTRMaterial *ttrMat, TessTTRContext *ttrCtx, AssetReference **ref, TStr *parentId) {
    TTRAssetRef aref;
    TStr *textureAssetId;
    TessAsset *texAsset;
    switch(ttrMat->shaderType) {
        case Shader_Type_Unlit_Textured_Cutout:
        case Shader_Type_Unlit_Textured:
        aref = ttrMat->albedoTexARef;
        textureAssetId = tess_asset_id_from_aref(ttrCtx->as, ttrCtx->tbl, ttrCtx->imTbl, aref, ttrCtx->package);
        *ref = add_asset_reference(ttrCtx->as, textureAssetId);
        texAsset = tess_force_load_asset(ttrCtx->as, textureAssetId);
        break;
        default:
        *ref = NULL;
        break;
    }

    msg->type = Render_Message_Material_Query;
    msg->matQ.materialId = 0;
    msg->matQ.shaderId = ttrMat->shaderType;
    switch(ttrMat->shaderType) {
        case Shader_Type_Unlit_Textured_Cutout:
        case Shader_Type_Unlit_Textured:
            //texAsset = tess_get_asset(as, textureAssetId);
            if(texAsset != NULL && texAsset->type == Tess_Asset_Texture) {
                TessTextureAsset *texa = (TessTextureAsset*)texAsset;
                msg->matQ.iData.unlitTextured.textureId = texa->textureId;
            } else {
                msg->matQ.iData.unlitTextured.textureId = 0;
            }
            msg->matQ.iData.unlitTextured.color = ttrMat->tintColor;
            break;
        case Shader_Type_Unlit_Fade:
            //texAsset = tess_get_asset(as, textureAssetId);
            if(texAsset != NULL && texAsset->type == Tess_Asset_Texture) {
                TessTextureAsset *texa = (TessTextureAsset*)texAsset;
                msg->matQ.iData.unlitFade.textureId = texa->textureId;
            } else {
                msg->matQ.iData.unlitFade.textureId = 0;
            }
            msg->matQ.iData.unlitFade.color = ttrMat->tintColor;
            break;
        case Shader_Type_Unlit_Color:
            msg->matQ.iData.unlitColor.color = ttrMat->tintColor;
            break;
        case Shader_Type_Gizmo:
            msg->matQ.iData.gizmoMat.color = ttrMat->tintColor;
            break;
        case Shader_Type_Unlit_Vertex_Color:
            msg->matQ.iData.unlitVertexColor.color = ttrMat->tintColor;
            break;
        default:
            fprintf(stderr, "Object material had unknown shader type: %d\n", ttrMat->shaderType);
            assert(0); // invalid material, only here for debug
            break;
    }
}

ASYNC TessObjectAsset* tess_load_object(TessTTRContext *tctx, TessLoadingAsset *lasset, TTRObject *tobj) {
    scheduler_assert_task(); // async function
    AsyncTask task;
    TessAsset *meshAsset, *colliderAsset;
    TessObjectAsset *tessObj = pool_allocate(tctx->as->objectPool);
    assert(tessObj);
    TTRAssetRef aref = tobj->meshARef;
    TStr *curPackage = tess_get_asset_package_from_id(tctx->as, lasset->assetId);

    TStr *meshAssetId = tess_asset_id_from_aref(tctx->as, tctx->tbl, tctx->imTbl, aref, curPackage);
    TStr *textureAssetId;
    if(!tctx->as->isServer) {
        tessObj->meshRef = add_asset_reference(tctx->as, meshAssetId);
        meshAsset = tess_force_load_asset(tctx->as, meshAssetId);
        assert(meshAsset != NULL);
        lasset->type = Tess_Asset_Object;
        meshAsset = tess_get_asset(lasset->as, meshAssetId);
        assert(meshAsset && meshAsset->type == Tess_Asset_Mesh);
        tessObj->asset.assetId = lasset->assetId;
        tessObj->mesh = (TessMeshAsset*)meshAsset;
    } else {
        tessObj->meshRef = NULL;
        tessObj->mesh = NULL;
    }

    aref = tobj->colliderARef;
    TStr *colliderAssetId = tess_asset_id_from_aref(tctx->as, tctx->tbl, tctx->imTbl, aref, curPackage);
    if(!TTR_AREF_EMPTY(aref) && tctx->as->physics != NULL) {
        tessObj->colliderRef = add_asset_reference(tctx->as, colliderAssetId);
        colliderAsset = tess_force_load_asset(tctx->as, colliderAssetId);
        assert(colliderAsset != NULL && colliderAsset->type == Tess_Asset_Collider);
        tessObj->collider = (TessColliderAsset*)colliderAsset;
    } else {
        tessObj->colliderRef = NULL;
        tessObj->collider = NULL;
    }

    tessObj->asset.type = Tess_Asset_Object;
    tess_finalize_asset(tctx->as, lasset, &tessObj->asset, lasset->assetId, Tess_Asset_Object);

    return tessObj;
}

ASYNC void tess_unload_object(TessAssetSystem *as, TessObjectAsset *obj) {
    scheduler_assert_task(); // this is an async function 

    if(obj->meshRef != NULL) {
        remove_asset_reference(as, obj->meshRef);
    }
    if(obj->colliderRef != NULL) {
        remove_asset_reference(as, obj->colliderRef);
    }
    tess_unlist_asset(as, &obj->asset);
    
    pool_free(as->objectPool, obj);
}

ASYNC TessColliderAsset* tess_load_collider_asset(TessAssetSystem *as, TessLoadingAsset *lasset, TTRCollider *collider) {
    TessColliderAsset *ret = malloc(sizeof(TessColliderAsset));

    // TODO: client needs physics too!
    if(as->physics != NULL) {
        TTRBuffer *sphereBuffer = TTR_REF_TO_PTR(TTRBuffer, collider->sphereBufRef);
        TTRBuffer *boxBuffer = TTR_REF_TO_PTR(TTRBuffer, collider->boxBufRef);
        TTRBuffer *vBuffer = TTR_REF_TO_PTR(TTRBuffer, collider->vertBufRef);
        TTRBuffer *iBuffer = TTR_REF_TO_PTR(TTRBuffer, collider->indexBufRef);
        TTRSphereCollider *spheres = (TTRSphereCollider*)sphereBuffer->data;
        TTRBoxCollider *boxes = (TTRBoxCollider*)boxBuffer->data;
        V3 *verts = (V3*)vBuffer->data;
        uint32_t *indices = (uint32_t*)iBuffer->data;
        int totalShapes = collider->numSpheres + collider->numBoxes + ((collider->numVertices > 0)?1:0);

        assert(collider->numBoxes < 100);
        assert(collider->numSpheres < 100);
        TTRBoxCollider *boxes2 = malloc(100*sizeof(TTRBoxCollider));
        TTRSphereCollider *spheres2 = malloc(100*sizeof(TTRSphereCollider));
        for(int i = 0; i < collider->numBoxes; i++) {
            boxes2[i].min.x = boxes[i].min.x;
            boxes2[i].min.y = boxes[i].min.z;
            boxes2[i].min.z = boxes[i].min.y;
            boxes2[i].max.x = boxes[i].max.x;
            boxes2[i].max.y = boxes[i].max.z;
            boxes2[i].max.z = boxes[i].max.y;
        }
        for(int i = 0; i < collider->numSpheres; i++) {
            spheres2[i].radius = spheres[i].radius;
            spheres2[i].pos.x = spheres[i].pos.x;
            spheres2[i].pos.y = spheres[i].pos.z;
            spheres2[i].pos.z = spheres[i].pos.y;
        }
        for(int i = 0; i < collider->numVertices; i++) {
            float temp = verts[i].y;
            verts[i].y = verts[i].z;
            verts[i].z = temp;
        }

        ret->shapeCount = totalShapes;
        ret->shapes = calloc(totalShapes, sizeof(CPxShape)); 
        int idx = 0;
        for(int i = 0; i < collider->numSpheres; i++) {
            // TODO: check bounds!
            auto s = spheres2 + i;
            ret->shapes[idx] = physx_create_sphere_shape(as->physics, s->pos, s->radius);
            idx++;
        }
        for(int i = 0; i < collider->numBoxes; i++) {
            auto b = boxes2 + i;
            ret->shapes[idx] = physx_create_box_shape(as->physics, b->min, b->max);
            idx++;
        }
        free(boxes2);
        free(spheres2);
        if(collider->numVertices > 0) {
            ret->shapes[idx] = physx_create_mesh_shape(as->physics, collider->numVertices, verts, collider->numIndices, indices);
            idx++;
        }
        assert(idx == totalShapes);
    } else {
        ret->shapes = NULL;
        ret->shapeCount = 0;
    }

    tess_finalize_asset(as, lasset, &ret->asset, lasset->assetId, Tess_Asset_Collider);
    return ret;
}

ASYNC void tess_unload_collider_asset(TessAssetSystem *as, TessColliderAsset *col) {
    scheduler_assert_task();

    int count = col->shapeCount;
    for(int i = 0; i < count; i++) {
        physx_release_shape(as->physics, col->shapes[i]);
    }
    col->shapeCount = 0;

    if(col->shapes != NULL) {
        free(col->shapes);
        col->shapes = NULL;
    }
    tess_unlist_asset(as, &col->asset);
    free(col);
}

ASYNC TessMapAsset* tess_load_map_asset(TessAssetSystem *as, TessLoadingAsset *lasset, TTRMap *map) {
    TTRMapObjectTable *mapObjTbl = TTR_REF_TO_PTR(TTRMapObjectTable, map->objectTableRef);
    TTRMapEntityTable *mapEntTbl = TTR_REF_TO_PTR(TTRMapEntityTable, map->entityTableRef);
    // TODO: use pool or something
    TessMapAsset *ret = malloc(sizeof(TessMapAsset));
    ret->mapObjectCount = mapObjTbl->numEntries;
    ret->mapEntityCount = mapEntTbl->numEntries;
    assert(ret);
    // TODO: no malloc
    ret->objects = malloc(sizeof(ret->objects[0]) * mapObjTbl->numEntries);
    assert(ret->objects);
    for(int i = 0; i < mapObjTbl->numEntries; i++) {
        TStr *assetId = tess_intern_string_s(as->tstrings, mapObjTbl->entries[i].assetId, sizeof(mapObjTbl->entries[i].assetId));
        ret->objects[i] = (TessMapObject) {
            .objectid = mapObjTbl->entries[i].objectId,
            .assetId = assetId
        };
    }
    // TODO: no malloc
    ret->entities = malloc(sizeof(ret->entities[0]) * mapEntTbl->numEntries);
    assert(ret->entities);
    for(int i = 0; i < mapEntTbl->numEntries; i++) {
        TTRMapEntityEntry *e = mapEntTbl->entries + i;
        ret->entities[i].position = e->position;
        ret->entities[i].rotation = e->rotation;
        ret->entities[i].scale = e->scale;
        ret->entities[i].objectId = e->objectId;
    }
    
    tess_finalize_asset(as, lasset, &ret->asset, lasset->assetId, Tess_Asset_Map);

    return ret;
}

ASYNC void tess_unload_map_asset(TessAssetSystem *as, TessMapAsset *map) {
    scheduler_assert_task(); // this is an async function 

    tess_unlist_asset(as, &map->asset);
    free(map->objects);
    free(map->entities);
    
    free(map);
}

ASYNC TessAsset* tess_load_asset(TessLoadingAsset *lasset) {
    AsyncTask task;
    TessAssetStatus status = tess_get_asset_status(lasset->as, lasset->assetId);
    // TODO: in case the status is InQueue, remove it from queue
    //printf("status %s %d\n", lasset->assetId->cstr, status);
    assert(status == Tess_Asset_Status_None || status == Tess_Asset_Status_Loading);
    tess_set_asset_status(lasset->as, lasset->assetId, Tess_Asset_Status_Loading);
    //printf("Load asset %s\n", lasset->assetId->cstr);
    TessAssetSystem *as = lasset->as;
    TStr *packageName = tess_get_asset_package_from_id(as, lasset->assetId);
    assert(lasset);
    char buf[AIKE_MAX_PATH];
    buf[0] = 0;
    // TODO: safe platform-independent path construction function
    strcpy(buf, "Packages/");
    strcat(buf, packageName->cstr);
    strcat(buf, "/");
    strcat(buf, lasset->fileName->cstr);
    // TODO: overflow protect?
    as->numOpenAssetFiles++;
    as->totalFileLoads++;
    tess_async_load_file(as->fileSystem, buf, &task);
    scheduler_wait_for(&task);
    //printf("Asset %s file ready \n", lasset->assetId->cstr);
    return tess_process_asset_from_ttr(as, task.file, lasset);
}

ASYNC void tess_unload_asset(TessAssetSystem *as, TStr *assetId) {
    TessAsset *asset = tess_get_asset(as, assetId);
    if(asset != NULL) {
        switch(asset->type) {
            case Tess_Asset_Mesh:
                tess_unload_mesh(as, (TessMeshAsset*)asset);
                break;
            case Tess_Asset_Texture:
                tess_unload_texture(as, (TessTextureAsset*)asset);
                break;
            case Tess_Asset_Object:
                tess_unload_object(as, (TessObjectAsset*)asset);
                break;
            case Tess_Asset_Map:
                tess_unload_map_asset(as, (TessMapAsset*)asset);
                break;
            case Tess_Asset_Collider:
                tess_unload_collider_asset(as, (TessColliderAsset*)asset);
                break;
            default:
                assert(0); // how did unknown asset type even get loaded?
                break;
        }
    }
}

ASYNC void tess_unlist_asset(TessAssetSystem *as, TessAsset *asset) {
    assert(tess_get_asset_status(as, asset->assetId) != Tess_Asset_Status_Loading);
    khiter_t k = kh_get(64, as->loadedAssetMap, (intptr_t)asset->assetId);
    assert(k != kh_end(as->loadedAssetMap));
    kh_del(64, as->loadedAssetMap, k);
    tess_set_asset_status(as, asset->assetId, Tess_Asset_Status_None);
    as->numLoadedAssets--;
}

ASYNC TessAsset* tess_force_load_asset(TessAssetSystem *as, TStr *assetId) {
    int dummy;
    //printf("Force load asset %s\n", assetId->cstr);
    // TODO: event based, no polling!
    int i = 0;
    while(tess_is_asset_loading(as, assetId)) {
        //printf("still loading.... %s\n", assetId->cstr);
        scheduler_yield();
    }
    if(tess_is_asset_loaded(as, assetId)) {
        //printf("Forced and already loaded %s\n", assetId->cstr);
        return tess_get_asset(as, assetId);
    }
    assert(!tess_is_asset_loaded(as, assetId));
    TStr *fileName;
    TStr *packageName = tess_get_asset_package_from_id(as, assetId);
    if(get_asset_file(as, assetId, &fileName)) {
        TessLoadingAsset *lasset;
        khiter_t k = kh_get(64, as->loadingAssetMap, (intptr_t)assetId);
        if(k == kh_end(as->loadingAssetMap)) {
            lasset = pool_allocate(as->loadingAssetPool);
            loading_asset_init(as, lasset, fileName, assetId);
        } else {
            assert(tess_get_asset_status(as, assetId) == Tess_Asset_Status_InQueue);
            lasset = kh_value(as->loadingAssetMap, k);
            scheduler_cancel_task(lasset->task);
            //printf("cancel %p (forced before dequeue)\r\n", lasset);
            tess_set_asset_status(as, assetId, Tess_Asset_Status_Loading);
        }
        //printf("Forced and begin load %s\n", assetId->cstr);
        TessAsset *ret = tess_load_asset(lasset);
        //printf("Forced and end load %s\n", assetId->cstr);
        return ret;
    } else {
        tess_set_asset_target_status(as, assetId, Tess_Asset_Status_Fail);
        return NULL;
    }
}

ASYNC void tess_task_load_asset(void *data) {
    scheduler_assert_task(); // async function

    TessLoadingAsset *lasset = (TessLoadingAsset*)data;
    lasset->task = NULL;
    //printf("load %p\r\n", data);
    // TODO: why is it here? (removing triggers assert!)
    tess_set_asset_status(lasset->as, lasset->assetId, Tess_Asset_Status_Loading);
    tess_load_asset(lasset);
    
    scheduler_task_end();
}

ASYNC void tess_task_unload_asset(void *data) {
    struct UnloadAssetData *ul = (struct UnloadAssetData*)data;
    TessAsset *asset = tess_get_asset(ul->as, ul->assetId);
    assert(asset != NULL); // unloading asset that doesn't exist!
    asset->ul = NULL;
    scheduler_assert_task();
    assert(tess_get_asset_status(ul->as, ul->assetId) == Tess_Asset_Status_Pending_Destroy);
    //printf("Unload task started for %s\n", ul->assetId->cstr);
    tess_unload_asset(ul->as, ul->assetId);
    free(ul); // TODO: get rid of this!
    scheduler_task_end();
}

ASYNC TessAsset* tess_process_asset_from_ttr(TessAssetSystem *as, TessFile *tfile, TessLoadingAsset *lasset)
{
    TessAsset *asset = NULL;
    scheduler_assert_task(); // this function can only be called from within a task
    lasset->file = tfile;

    void* fileBuf = tfile->req.buffer;
    assert(tfile->req.fileOffset == 0);
    uint64_t fileSize = tfile->req.nBytes;

    // TODO: check everything before you read it!!!!!!
    auto header = (TTRHeader*)fileBuf;
    TessTTRContext tctx = (TessTTRContext) {
        .as = as,
        .header = header,
        .tbl = TTR_REF_TO_PTR(TTRDescTbl, header->descTblRef),
        .imTbl = TTR_REF_TO_PTR(TTRImportTbl, header->importTblRef),
        .package = tess_get_asset_package_from_id(as, lasset->assetId),
    };
    // TODO: maybe this should be cached in TessAsset?
    TStr *assetName = tess_get_asset_name_from_id(as, lasset->assetId);
    bool found = false;
    for(int i = 0; i < tctx.tbl->entryCount; i++)
    {
        TTRDescTblEntry *entry = &tctx.tbl->entries[i];
        char nameBuf[TTR_MAX_NAME_LEN];
        tess_replace_id_name_characters(nameBuf, entry->assetName, ARRAY_COUNT(nameBuf));
        TStr *internedName = tess_intern_string_s(as->tstrings, nameBuf, TTR_MAX_NAME_LEN);

        // NOTE: if there would be any dependencies they would also be kicked off here

        if(internedName == assetName)
        {
            switch(entry->type) 
            {
                case 'HSEM': // MESH // TODO: multi byte characters are endianness sensitive
                    {
                        lasset->type = Tess_Asset_Mesh;
                        TTRMesh *tmesh = TTR_REF_TO_PTR(TTRMesh, tctx.tbl->entries[i].ref);
                        asset = (TessAsset*)tess_load_mesh(&tctx, lasset, tmesh);
                        found = true;
                    } break;
                case ' JBO': // OBJ
                    {
                        TTRObject *tobject = TTR_REF_TO_PTR(TTRObject, tctx.tbl->entries[i].ref);
                        asset = (TessAsset*)tess_load_object(&tctx,lasset, tobject);
                        found = true;
                    }break;
                case ' XET': // TEX
                    {
                        TTRTexture *ttex = TTR_REF_TO_PTR(TTRTexture, tctx.tbl->entries[i].ref);
                        lasset->type = Tess_Asset_Texture;
                        asset = (TessAsset*)tess_load_texture(as, lasset, ttex);
                        found = true;
                    } break;
                // NOTE: loading map asset does not load the map
                // map asset is only a container of data that is
                // further handled by a different subsystem
                case ' PAM': // MAP
                    {
                        TTRMap *tmap = TTR_REF_TO_PTR(TTRMap, tctx.tbl->entries[i].ref);
                        lasset->type = Tess_Asset_Map;
                        asset = (TessAsset*)tess_load_map_asset(as, lasset, tmap);
                        found = true;
                    } break;
                case ' LOC':
                    {
                        TTRCollider *tcol = TTR_REF_TO_PTR(TTRCollider, tctx.tbl->entries[i].ref);
                        lasset->type = Tess_Asset_Collider;
                        asset = (TessAsset*)tess_load_collider_asset(as, lasset, tcol);
                        found = true;
                    } break;
                default:
                    assert(0);
                    fprintf(stderr, "Trying to load unknown asset type!\n");
                    break;
            }
        }
        if(found)
            break;
    }
    if(!found)
    {
        // nothing was loaded
        // TODO: log error or something??
        fprintf(stderr, "TTR file %s loaded, but asset %s was not found in it!\n", tfile->filePath, lasset->assetId->cstr);
    }
    return asset;
}

// TODO: invalid Aref results in segfault, add protection
TStr* tess_asset_id_from_aref(TessAssetSystem *as, TTRDescTbl *tbl, TTRImportTbl *imTbl, TTRAssetRef aref, TStr *currentPackage)
{
    uint32_t meshEntryIdx = (aref.tblIndex & ~TTR_AREF_EXTERN_MASK);
    TStr *meshAssetName, *meshPackageName;
    // special code for unset asset ids
    if(TTR_AREF_EMPTY(aref)) {
        return as->tstrings->empty;
    }
    if(TTR_IS_EXTERN_AREF(aref))
    {
        TTRImportTblEntry *imEnt = &imTbl->entries[meshEntryIdx];
        meshAssetName = tess_intern_string_s(as->tstrings, imEnt->assetName, TTR_MAX_NAME_LEN);
        meshPackageName = tess_intern_string_s(as->tstrings, imEnt->packageName, TTR_MAX_NAME_LEN);
    }
    else
    {
        meshAssetName = tess_intern_string_s(as->tstrings, tbl->entries[meshEntryIdx].assetName, TTR_MAX_NAME_LEN);
        meshPackageName = currentPackage;
    }
    TStr *meshAssetId = tess_get_asset_id(as, meshPackageName, meshAssetName);
    return meshAssetId;
}


TStr *tess_get_asset_id(TessAssetSystem *as, TStr *package, TStr *asset)
{
    char buf[TTR_MAX_NAME_LEN*2 + 2];
    assert(package->len + asset->len + 2 < ARRAY_COUNT(buf));
    strcpy(buf, package->cstr);
    strcat(buf, "/");
    strcat(buf, asset->cstr);
    return tess_intern_string_s(as->tstrings, buf, ARRAY_COUNT(buf));
}

TStr *tess_get_asset_name_from_id(TessAssetSystem *as, TStr *assetId)
{
    const char *name = strrchr(assetId->cstr, '/');
    assert(name);
    assert(strlen(name) > 1);
    return tess_intern_string(as->tstrings, name + 1);
}

TStr *tess_get_asset_package_from_id(TessAssetSystem *as, TStr *assetId)
{
    const char *name = strrchr(assetId->cstr, '/');
    assert(name);
    uint32_t len = name - assetId->cstr;
    TStr *ret = tess_intern_string_s(as->tstrings, assetId->cstr, len);
    return ret;
}

internal void loading_asset_init(TessAssetSystem *as, TessLoadingAsset *lasset, TStr *fileName, TStr *assetId) {
    int dummy;
    assert(lasset);
    lasset->fileName = fileName;
    lasset->as = as;
    lasset->file = NULL;
    lasset->assetId = assetId;
    lasset->type = Tess_Asset_Unknown;
    lasset->task = NULL;
    khiter_t k = kh_get(64, as->loadingAssetMap, (intptr_t)assetId);
    assert(k == kh_end(as->loadingAssetMap));
    k = kh_put(64, as->loadingAssetMap, (intptr_t)assetId, &dummy);
    kh_value(as->loadingAssetMap, k) = lasset;
    as->numLoadingAssets++;
}

internal bool tess_queue_asset(TessAssetSystem *as, TStr *assetId)
{
    int dummy;
    if(tess_is_asset_loaded(as, assetId))
        return true;
    if(tess_is_asset_loading(as, assetId))
        return false;
    
    //printf("Queue asset %s\n", assetId->cstr);

    TStr *fileName;
    TStr *packageName = tess_get_asset_package_from_id(as, assetId);

    // should not queue asset that does not need to be loaded! (use add_asset_reference() to manually load assets)
    assert(tess_get_asset_target_status(as, assetId) != Tess_Asset_Status_None);

    if(get_asset_file(as, assetId, &fileName)) {
        TessLoadingAsset *lasset = pool_allocate(as->loadingAssetPool);
        loading_asset_init(as, lasset, fileName, assetId);
        tess_set_asset_status(as, assetId, Tess_Asset_Status_InQueue);
        lasset->task = scheduler_queue_task(tess_task_load_asset, (void*)lasset);
        // TODO: instead of immediately loading it, we just wait till the asset system picks it up itself
    }
    else {
        tess_set_asset_target_status(as, assetId, Tess_Asset_Status_Fail);
        printf("FAILED TO LOAD ASSET %s\n", assetId->cstr);
    }
    return false;
}

TessAsset* tess_get_asset(TessAssetSystem *as, TStr *assetId)
{
    khiter_t k = kh_get(64, as->loadedAssetMap, (intptr_t)assetId);
    if(k == kh_end(as->loadedAssetMap))
        return NULL;
    TessAsset *ret = kh_value(as->loadedAssetMap, k);
    assert(ret != NULL);
    return ret;
}

internal void asset_loaded(TessAssetSystem *as, TessAsset *asset)
{
    int dummy;
    assert(asset);
    khiter_t k = kh_put(64, as->loadedAssetMap, (intptr_t)asset->assetId, &dummy);
    kh_value(as->loadedAssetMap, k) = asset;

    DELEGATE_INVOKE(as->onAssetLoaded, as, asset);
}

internal void tess_loading_asset_done(TessAssetSystem *as, TessLoadingAsset *lasset, TessAsset *asset, TessAssetStatus newStatus, bool completed)
{
    khiter_t k;
    // asset loaded!
    if(completed) {
        assert(lasset->file);
        as->numOpenAssetFiles--;
        tess_unload_file(as->fileSystem, lasset->file);
        lasset->file = NULL;
        asset_loaded(as, asset);
    }

    as->numLoadingAssets--;
    assert(as->numLoadingAssets >= 0);


    // set new status
    tess_set_asset_status(as, lasset->assetId, newStatus);
    assert(newStatus != Tess_Asset_Status_Loading);

    //uint32_t count = buf_len(as->loadingAssets);
    k = kh_get(64, as->loadingAssetMap, (intptr_t)lasset->assetId);
    assert(k != kh_end(as->loadingAssetMap));
    kh_del(64, as->loadingAssetMap, k);
    pool_free(as->loadingAssetPool, lasset);
}

bool tess_are_all_loads_complete(TessAssetSystem *as)
{
    return as->numLoadingAssets == 0;
}

#define STRUCT_IN_RANGE(start, size, ptr) ((uint8_t*)(ptr) >= (uint8_t*)(start) && ((uint8_t*)(ptr) + sizeof(*(ptr))) <= ((uint8_t*)(start)+(size)))
#define ARRAY_IN_RANGE(start, size, array, len) ((uint8_t*)(array) >= (uint8_t*)(start) && ((uint8_t*)(array) + sizeof((array)[0])*len) <= ((uint8_t*)(start)+(size)))

// all characters except A-z and 0-9 get replaced with '-'
// the intention of this is to make id names safe so we don't have to worry about
// someone hacking around with / /n /t %s and stuff like that
// maybe it's worth not being lazy here?
internal void tess_replace_id_name_characters(char buf[], const char *name, uint32_t buflen)
{
    for(int i = 0; i < buflen; i++)
    {
        if(!name[i])
        {
            buf[i] = 0;
            return;
        }
        if((name[i] >= 'A' && name[i] <= 'z') || (name[i] >= '0' && name[i] <= '9'))
            buf[i] = name[i];
        else
            buf[i] = '-';
    }
    fprintf(stderr, "string passed to tess_escape_id_name() wasn't null terminated!\n");
    assert(false);
}

TStr* intern_asset_id(TessAssetSystem *as, const char *package, const char *asset) {
    char nameBuf[TTR_MAX_NAME_LEN];
    tess_replace_id_name_characters(nameBuf, asset, ARRAY_COUNT(nameBuf));
    // TODO: no reason to intern name and package, do it directly
    TStr *internedName = tess_intern_string_s(as->tstrings, nameBuf, TTR_MAX_NAME_LEN);
    tess_replace_id_name_characters(nameBuf, package, ARRAY_COUNT(nameBuf));
    TStr *internedPackage = tess_intern_string_s(as->tstrings, nameBuf, TTR_MAX_NAME_LEN);
    return tess_get_asset_id(as, internedPackage, internedName);
}

void tess_refresh_package_list(TessAssetSystem *as)
{
    char pdirPath[AIKE_MAX_PATH];
    AikeFileEntry entries[64];
    int count;
    AikePlatform *platform = as->fileSystem->platform;
    strcpy(pdirPath, "Packages");
    AikeDirectory *dir = platform->open_directory(platform, pdirPath);
    if(!dir)
    {
        fprintf(stderr, "Unable to read 'Packages' directory\n");
        exit(-1);
    }
    buf_clear(as->packageList);
    while((count = platform->next_files(platform, dir, entries, ARRAY_COUNT(entries))))
    {
        for(int i = 0; i < count; i++)
        {
            if(entries[i].type == Aike_File_Entry_Directory)
            {
                TStr *packageName = tess_intern_string(as->tstrings, entries[i].name);
                buf_push(as->packageList, packageName);
            }
        }
    }
    platform->close_directory(platform, dir);
}

void tess_gen_lookup_cache_for_package(TessAssetSystem *as, TStr *packageName)
{
    char pdirPath[AIKE_MAX_PATH];
    AikeFileEntry entries[64];
    int count;
    int dummy;
    AikePlatform *platform = as->fileSystem->platform;

    // have we already created a cache for this package?
    // TODO: if packageName is interned then we don't even need string keys!
    khiter_t key = kh_get(str, as->packageAssetMap, packageName->cstr);
    if(key != kh_end(as->packageAssetMap))
    {
        printf("Package cache for %s already exists!\n", packageName->cstr);
        return;
    }

    AssetLookupCache *cache = pool_allocate(as->assetLookupCachePool);
    assert(cache);
    cache->entries = NULL;
    key = kh_put(str, as->packageAssetMap, packageName->cstr, &dummy);
    kh_value(as->packageAssetMap, key) = cache;

    strcpy(pdirPath, "Packages/");
    strcat(pdirPath, packageName->cstr);
    AikeDirectory *dir = platform->open_directory(platform, pdirPath);
    if(!dir)
        return;
    while((count = platform->next_files(platform, dir, entries, ARRAY_COUNT(entries))))
    {
        for(int i = 0; i < count; i++)
        {
            char *extension = strrchr(entries[i].name, '.');
            bool isttr = extension && strcmp(extension, ".ttr") == 0;
            if(entries[i].type == Aike_File_Entry_File && isttr)
            {
                strcpy(pdirPath, "Packages/");
                strcat(pdirPath, packageName->cstr);
                strcat(pdirPath, "/");
                strcat(pdirPath, entries[i].name);
                AikeMemoryBlock *block = platform->map_file(platform, pdirPath, 0, 0);
                TTRHeader *hdr = (TTRHeader*)block->memory;
                if(!STRUCT_IN_RANGE(block->memory, block->size, hdr))
                    goto unmap_file_and_continue; // missing header
                TTRDescTbl *dtbl = TTR_REF_TO_PTR(TTRDescTbl, hdr->descTblRef);
                if(!STRUCT_IN_RANGE(block->memory, block->size, dtbl))
                    goto unmap_file_and_continue; // corrupt header/descTbl
                if(!ARRAY_IN_RANGE(block->memory, block->size, dtbl->entries, dtbl->entryCount))
                    goto unmap_file_and_continue; // corrupt header/descTbl

                for(int j = 0; j < dtbl->entryCount; j++)
                {
                    char ebuf[TTR_MAX_NAME_LEN];
                    tess_replace_id_name_characters(ebuf, dtbl->entries[j].assetName, TTR_MAX_NAME_LEN);
                    AssetLookupEntry *entry = pool_allocate(as->assetLookupEntryPool);
                    assert(entry);
                    entry->packageName = packageName;
                    entry->fileName = tess_intern_string(as->tstrings, entries[i].name);
                    entry->assetName = tess_intern_string(as->tstrings, ebuf);
                    //printf("asset %s\n", ebuf);
                    entry->assetType = dtbl->entries[j].type;
                    buf_push(cache->entries, entry);
                }

unmap_file_and_continue:
                platform->unmap_file(platform, block);
            }
        }
    }
    platform->close_directory(platform, dir);
}

bool get_asset_file(TessAssetSystem *as, TStr *assetId, TStr **result)
{
    TStr *package = tess_get_asset_package_from_id(as, assetId);
    TStr *assetName = tess_get_asset_name_from_id(as, assetId);
    khiter_t k = kh_get(str, as->packageAssetMap, package->cstr);
    if(k == kh_end(as->packageAssetMap))
    {
        fprintf(stderr, "get_asset_file() called for package %s, but no cache for that package!\n", package->cstr);
        return false;
    }
    AssetLookupCache *cache = (AssetLookupCache*)kh_value(as->packageAssetMap, k);
    int count = buf_len(cache->entries);
    // TODO: if a package has many assets, linear search is slow!
    // binary search could be good enough because the cache is static
    // (uses less memory than hash map per package)
    for(int i = 0; i < count; i++)
    {
        if(assetName->cstr == cache->entries[i]->assetName->cstr)
        {
            *result = cache->entries[i]->fileName;
            return true;
        }
    }
    return false;
}

AssetReference* add_asset_reference(TessAssetSystem *as, TStr *assetId)
{
    int putret;
    khiter_t k = kh_get(ptrToU32, as->refCountMap, (intptr_t)assetId);
    if(k == kh_end(as->refCountMap)) {
        k = kh_put(ptrToU32, as->refCountMap, (intptr_t)assetId, &putret);
        kh_val(as->refCountMap, k) = 0;
        assert(putret != 0); // already exists in map (but we just checked and it didnt?)
        tess_set_asset_target_status(as, assetId, Tess_Asset_Status_Loaded);
    }
    int refs = ++kh_val(as->refCountMap, k);
    AssetReference *ret = pool_allocate(as->assetRefPool);
    ret->assetId = assetId;
    return ret;
}

void remove_asset_reference(TessAssetSystem *as, AssetReference *ref)
{
    khiter_t k = kh_get(ptrToU32, as->refCountMap, (intptr_t)ref->assetId);
    assert(k != kh_end(as->refCountMap)); // asset is not referenced, why do you still have a reference?
    int result = --kh_val(as->refCountMap, k);
    //printf("%s now has %d refs\n", ref->assetId->cstr, result);
    assert(result >= 0); // invalid state, it should be deleted from map if it was already 0
    if(result == 0) {
        kh_del(ptrToU32, as->refCountMap, k);
        tess_set_asset_target_status(as, ref->assetId, Tess_Asset_Status_None);
    }
    ref->assetId = NULL;
    pool_free(as->assetRefPool, ref);
}
