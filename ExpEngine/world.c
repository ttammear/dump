
// define new object
void tess_register_object(TessGameSystem *gs, uint32_t id, TStr *assetId)
{
    assert(id > 0 && id < ARRAY_COUNT(gs->objectTable));
    TessObject *obj = &gs->objectTable[id];
    if(BIT_IS_SET(obj->flags, Tess_Object_Flag_Registered))
    {
        fprintf(stderr, "Overriding previous object ID: %d old: %s new: %s\n", id, obj->assetId->cstr, assetId->cstr);
    }
    obj->assetId = assetId;
    obj->id = id;
    obj->asset = NULL;
    SET_BIT(obj->flags, Tess_Object_Flag_Registered);
    CLEAR_BIT(obj->flags, Tess_Object_Flag_Loaded);
    
    // TODO: remove once you can load assets on demand
    tess_load_asset_if_not_loaded(gs->assetSystem, assetId);
}

// set all defined object assets
void tess_temp_assign_all(TessGameSystem *gs)
{
    printf("FIXME: tess_temp_assign_all\n");
    int count = ARRAY_COUNT(gs->objectTable);
    for(int i = 0; i < count; i++)
    {
        TessObject *obj = &gs->objectTable[i];
        if(BIT_IS_SET(obj->flags, Tess_Object_Flag_Registered) &&
                !BIT_IS_SET(obj->flags, Tess_Object_Flag_Loaded))
        {
            TessAsset *asset = tess_get_asset(gs->assetSystem, obj->assetId);
            if(asset != NULL && asset->type == Tess_Asset_Object)
            {
                gs->objectTable[i].asset = (TessObjectAsset*)asset;
                SET_BIT(obj->flags, Tess_Object_Flag_Loaded);
            }
            else if(asset == NULL)
                fprintf(stderr, "Asset %s was not loaded!\n", obj->assetId->cstr);
            else
                fprintf(stderr, "Asset %s was not an object!\n", obj->assetId->cstr);
        }
    }
}

void tess_unregister_object(TessGameSystem *gs, uint32_t id)
{
    TessObject *obj = &gs->objectTable[id];
    assert(obj->id == id);
    obj->assetId = NULL;
    CLEAR_BIT(obj->flags, Tess_Object_Flag_Registered);
}

uint32_t tess_create_entity(TessGameSystem *gs, uint32_t id, Mat4 *modelMatrix)
{
    assert(id > 0 && id < ARRAY_COUNT(gs->objectTable));
    TessObject *obj = &gs->objectTable[id];
    assert(BIT_IS_SET(obj->flags, Tess_Object_Flag_Registered));
    TessEntity *entity = pool_allocate(gs->entityPool);
    assert(entity); // TODO: ??
    entity->id = entity - gs->entityPool;
    entity->objectId = id;
    entity->objectToWorld = *modelMatrix;
    buf_push(gs->activeEntities, entity);
    return entity->id;
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

/*void tess_clip_to_world_pos(TessCamera *cam, V3 clipPos, V3 screenPos)
{
    Mat4 clipToView;
    inverse_perspective(&inv_persp, &cam->viewToClip);

    Mat4 viewToWorld;
    mat4_tr(&viewToWorld, cam->position, cam->rotation);

    Mat4 worldToClip;
    mat4_mul(&worldToClip, &viewToWorld, &clipToView);
}*/

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
        assert(BIT_IS_SET(obj->flags, Tess_Object_Flag_Loaded));
        uint32_t meshId = obj->asset->mesh->meshId;
        uint32_t materialId = obj->asset->materialId;
        render_system_render_mesh(gs->renderSystem, meshId, materialId, ent->id, &ent->objectToWorld);
    }
}

void tess_reset_world(TessGameSystem *gs)
{
    pool_clear(gs->entityPool);
    buf_clear(gs->activeEntities);
}
