void world_asset_loaded(TessGameSystem *gs, TessAssetSystem *as, TessAsset *asset);

void tess_world_init(TessGameSystem *gs)
{
    // seed null object
    TessObject *obj = &gs->objectTable[0];
    SET_BIT(obj->flags, Tess_Object_Flag_Loaded);
    obj->assetId = gs->tstrings->empty;
    obj->id = 0;
    obj->asset = &gs->assetSystem->nullObject;

    for(int i = 0; i < ARRAY_COUNT(gs->objectTable); i++)
    {
        obj = &gs->objectTable[i];
        obj->assetId = gs->tstrings->empty;
        obj->id = i;
        obj->asset = &gs->assetSystem->nullObject;
        SET_BIT(obj->flags, Tess_Object_Flag_Loaded);
    }

    DELEGATE_LISTEN(gs->assetSystem->onAssetLoaded, (OnAssetLoaded_t) world_asset_loaded, gs);
}

// define new object in object table id->asset
void tess_register_object(TessGameSystem *gs, uint32_t id, TStr *assetId)
{
    if(id == 0) // id 0 is unset object and can not be overriden
        return;
    assert(id < ARRAY_COUNT(gs->objectTable));
    TessObject *obj = &gs->objectTable[id];
    obj->assetId = assetId;
    obj->id = id;
    
    // TODO: remove once you can load assets on demand
    //bool loaded = tess_queue_asset(gs->assetSystem, assetId);

    obj->ref = add_asset_reference(gs->assetSystem, assetId);
    bool loaded = tess_is_asset_loaded(gs->assetSystem, assetId);
    
    if(loaded)
    {
        auto asset = tess_get_asset(gs->assetSystem, assetId);
        if(asset->type == Tess_Asset_Object)
        {
            SET_BIT(obj->flags, Tess_Object_Flag_Loaded);
            obj->asset = (TessObjectAsset*)asset;
        }
        else
            loaded = false;
    }
    if(!loaded)
    {
        CLEAR_BIT(obj->flags, Tess_Object_Flag_Loaded);
        obj->asset = &gs->assetSystem->nullObject;
    }
}

void create_physics_body(TessPhysicsSystem *p, TessEntity *ent, TessColliderAsset *col) {
    void *usrPtr = (void*)((uintptr_t)ent->id | 0x80000000);
    auto actor = physx_create_static_body_with_shapes(p, ent->pos, ent->rot, ent->scale, col->shapeCount, col->shapes, usrPtr);
    physx_add_body_to_scene(p, actor);
    //printf("entity at %f %f %f\r\n", ent->pos.x, ent->pos.y, ent->pos.z);
}

void entity_loaded(TessGameSystem *gs, TessEntity *entity, TessObjectAsset *asset) {
    auto col = asset->collider;
    if(gs->physics != NULL && col != NULL) {
        // TODO: this is never deleted
        create_physics_body(gs->physics, entity, col);
    }
}

void entity_destroyed(TessGameSystem *gs, TessEntity *entity) {

}

void world_asset_loaded(TessGameSystem *gs, TessAssetSystem *as, TessAsset *asset)
{
    // @OPTIMIZE (create mapping assetId -> objectids)
    // TODO: O(N^2) is not acceptable!!!!
    // NOTE: maybe a linked list starting from object->entity->entity.... ?
    // would have to be doubly linked list though... (for removal)
    // (a list per object doesn't sound attractive because most objects will be unique)
    int count = ARRAY_COUNT(gs->objectTable);
    for(int i = 0; i < count; i ++)
    {
        TessObject *obj = &gs->objectTable[i];
        if(!BIT_IS_SET(obj->flags, Tess_Object_Flag_Loaded) 
                && asset->type == Tess_Asset_Object
                && obj->assetId == asset->assetId)
        {
            auto col = ((TessObjectAsset*)asset)->collider;
            for(int j = 0; j < buf_len(gs->activeEntities); j++) {
                auto ent = gs->activeEntities[j];
                if(ent->objectId == i) {
                    entity_loaded(gs, ent, (TessObjectAsset*)asset);
                }
            }
            gs->objectTable[i].asset = (TessObjectAsset*)asset;
            SET_BIT(obj->flags, Tess_Object_Flag_Loaded);
        }
    }
}

void tess_unregister_object(TessGameSystem *gs, uint32_t id)
{
    TessObject *obj = &gs->objectTable[id];
    assert(obj->id == id);
    if(obj->ref != NULL) {
        remove_asset_reference(gs->assetSystem, obj->ref);
        obj->ref = NULL;
    }
    obj->assetId = gs->tstrings->empty;
    obj->asset = &gs->assetSystem->nullObject;
    SET_BIT(obj->flags, Tess_Object_Flag_Loaded); // the null object is loaded
}

uint32_t tess_create_entity(TessGameSystem *gs, uint32_t id, V3 pos, Quat rot, V3 scale, Mat4 *modelMatrix)
{
    assert(id < ARRAY_COUNT(gs->objectTable));
    TessObject *obj = &gs->objectTable[id];
    TessEntity *entity = pool_allocate(gs->entityPool);
    assert(entity); // TODO: ??
    entity->id = entity - gs->entityPool;
    entity->objectId = id;
    entity->objectToWorld = *modelMatrix;
    entity->pos = pos;
    entity->rot = rot;
    entity->scale = scale;
    entity->physicsActor = NULL;
    buf_push(gs->activeEntities, entity);
    if(obj->asset != &gs->assetSystem->nullObject) {
        entity_loaded(gs, entity, obj->asset);
    }
    return entity->id;
}

void tess_destroy_entity(TessGameSystem *gs, uint32_t id)
{
    auto entity = tess_get_entity(gs, id);
    if(entity->physicsActor != NULL) {
        physx_destroy_body(gs->physics, entity->physicsActor);
    }
    int index = buf_find_idx(gs->activeEntities, entity);
    assert(index != -1);
    buf_remove_at(gs->activeEntities, index);
    pool_free(gs->entityPool, entity);
}

TessEntity* tess_get_entity(TessGameSystem *gs, uint32_t id)
{
    if(id < pool_cap(gs->entityPool))
        return gs->entityPool + id;
    else
        return NULL;
}

TessObject *tess_get_object(TessGameSystem *gs, uint32_t id)
{
    if(id < ARRAY_COUNT(gs->objectTable))
        return gs->objectTable + id;
    else
        return NULL;
}

void tess_update_camera_perspective(TessCamera *cam)
{
    mat4_perspective(&cam->viewToClip, cam->FOV, cam->aspectRatio, cam->nearPlane, cam->farPlane);
}

V3 tess_world_to_clip_pos(TessCamera *cam, V3 worldPos)
{
    V3 camPos = cam->position;
    Quat camRot = cam->rotation;
    V3 camInvTranslate = make_v3(-camPos.x, -camPos.y, -camPos.z);
    Quat camInvRotation = make_quat(-camRot.w, camRot.x, camRot.y, camRot.z);
    Mat4 worldToView;
    mat4_rt(&worldToView, camInvRotation, camInvTranslate);
    Mat4 worldToClip;
    mat4_mul(&worldToClip, &cam->viewToClip, &worldToView);
    V4 result;
    mat4_v4_mul(&result, &worldToClip, make_v4(worldPos.x, worldPos.y, worldPos.z, 1.0f));
    return make_v3(result.x/result.w, result.y/result.w, result.z/result.w);
}

void tess_render_entities(TessGameSystem *gs)
{
    PROF_BLOCK();
    V3 camPos = gs->activeCamera->position;
    Quat camRot = gs->activeCamera->rotation;
    V3 camInvTranslate = make_v3(-camPos.x, -camPos.y, -camPos.z);
    Quat camInvRotation = make_quat(-camRot.w, camRot.x, camRot.y, camRot.z);
    Mat4 worldToView;
    mat4_rt(&worldToView, camInvRotation, camInvTranslate);

    mat4_mul(&gs->renderSystem->worldToClip, &gs->activeCamera->viewToClip, &worldToView);

    int count = buf_len(gs->activeEntities);
    for(int i = 0; i < count; i++)
    {
        TessEntity *ent = gs->activeEntities[i];
        TessObject *obj = &gs->objectTable[ent->objectId];
        bool objLoaded = BIT_IS_SET(obj->flags, Tess_Object_Flag_Loaded);
        if(!objLoaded) // use null object if not loaded
            obj = &gs->objectTable[0];
        uint32_t meshId, materialId;
        meshId = obj->asset->mesh->meshId;

        render_system_render_mesh(gs->renderSystem, meshId, ent->id, &ent->objectToWorld);
    }
}

void tess_reset_world(TessGameSystem *gs)
{
    for(int i = 0; i < buf_len(gs->activeEntities); i++) {
        tess_destroy_entity(gs, gs->activeEntities[i]->id);
    }
    for(int i = 0; i < TESS_MAX_OBJECTS; i++) {
        tess_unregister_object(gs, i);
    }
    pool_clear(gs->entityPool);
    buf_clear(gs->activeEntities);
}
