
void render_system_init(struct TessRenderSystem *rs)
{
    rs->viewSwapBuffer = aligned_alloc(_Alignof(struct SwapBuffer), sizeof(struct SwapBuffer));
    swap_buffer_init(rs->viewSwapBuffer);

    rs->gameRenderView = take_view_buffer(rs->viewSwapBuffer);

    rs->viewBuilder = malloc(sizeof(struct RenderViewBuilder));
    rview_builder_init(rs->viewBuilder);
}

void render_system_destroy(struct TessRenderSystem *rs)
{
    rview_builder_destroy(rs->viewBuilder);
    free(rs->viewBuilder);

    swap_buffer_destroy(rs->viewSwapBuffer);
    free(rs->viewSwapBuffer);
}

void process_render_messages(struct TessRenderSystem *rs)
{
    struct Renderer *renderer = rs->renderer;
    RenderMessage msg;
    while(renderer_next_message(renderer, &msg))
    {
        switch(msg.type)
        {
            case Render_Message_Mesh_Query_Result:
                if(msg.meshQueryResult.onComplete != NULL)
                    msg.meshQueryResult.onComplete(renderer, &msg.meshQueryResult, msg.meshQueryResult.userData);
                break;
            case Render_Message_Texture_Query_Response:
                if(msg.texQR.onComplete != NULL)
                    msg.texQR.onComplete(renderer, &msg.texQR, msg.texQR.userData);
                break;
            case Render_Message_Mesh_Ready:
                if(msg.meshR.onComplete != NULL)
                    msg.meshR.onComplete(renderer, &msg.meshR, msg.meshR.userData);
                else
                    printf("mesh ready %d\n", msg.meshR.meshId);
                break;
            case Render_Message_Texture_Ready:
                if(msg.texR.onComplete != NULL)
                    msg.texR.onComplete(renderer, &msg.texR, msg.texR.userData);
                else
                    printf("texture ready %d\n", msg.texR.textureId);
                break;
            case Render_Message_Material_Ready:
                printf("material ready %d\n", msg.matR.materialId);
                break;
            case Render_Message_Sample_Object_Ready:
                if(msg.sampleOR.onComplete != NULL)
                    msg.sampleOR.onComplete(renderer, &msg.sampleOR, msg.sampleOR.userData);
                break;
        }
    }
}

void render_system_screen_resize(struct TessRenderSystem *rs, float width, float height)
{
    RenderMessage msg = {};
    msg.type = Render_Message_Screen_Resize;
    msg.screenR.width = width;
    msg.screenR.height = height;
    renderer_queue_message(rs->renderer, &msg);
}

void render_system_begin_update(struct TessRenderSystem *rs)
{
    PROF_BLOCK();
    rview_builder_reset(rs->viewBuilder);
}

void render_system_render_mesh(struct TessRenderSystem *rs, uint32_t meshId, uint32_t objectId, struct Mat4 *objectToWorld)
{
    uint32_t materialId = 2;
    uint32_t data = 0;
    add_mesh_instance(rs->viewBuilder, meshId, materialId, objectToWorld, &data, 0, objectId);
}

void render_system_end_update(struct TessRenderSystem *rs)
{
    PROF_BLOCK();
    rs->viewBuilder->viewProjection = rs->worldToClip;
    build_view(rs->viewBuilder, rs->gameRenderView);
    rs->gameRenderView = swap_view_for_newer(rs->viewSwapBuffer, rs->gameRenderView);
}
