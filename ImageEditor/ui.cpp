
static bool context_menu_create_proc(void* ptr);
static bool context_menu_leftc_close_proc(void *ptr);
static void layout_tree_split_node(LayoutTree *tree, LayoutTreeNode *node, float firstWeight, bool horizontal);

AikeViewState* ui_alloc_view(UserInterface *ui)
{
    assert(ui->numFreeViews > 0);
    int32_t freeIdx = --ui->numFreeViews;
    uint32_t poolIdx = ui->viewFreeList[freeIdx];
    assert(poolIdx < ARRAY_COUNT(ui->viewFreeList));
#ifdef AIKE_DEBUG
    ui->viewFreeList[freeIdx] = 0xFFFFFFFF;
#endif
    AikeViewState *ret = &ui->viewPool[poolIdx];
    ret->type = AikeViewState::Unknown;
    return ret;
}

void ui_free_view(UserInterface *ui, AikeViewState *view)
{
    int32_t index = view - ui->viewPool;
    assert(index >= 0 && index < ARRAY_COUNT(ui->viewPool));
    uint32_t freeIdx = ui->numFreeViews++;
    assert(freeIdx < ARRAY_COUNT(ui->viewFreeList));
    ui->viewFreeList[freeIdx] = index;
} 

static void image_view_init(ImageView *imgView)
{
    imgView->offset = Vec2(0.0f, 0.0f);
    imgView->scale1000 = 1000;
    imgView->imageSpaceCenter = Vec2(600.0f, 600.0f);
    imgView->grabbing = false;
}

static void init_view(uint32_t viewType, AikeViewState *view)
{
    switch(viewType)
    {
        case AikeViewState::Unknown:
            fprintf(stderr, "Init unknown view (%d)\n", viewType);
            break;
        case AikeViewState::ImageView:
            image_view_init(&view->imgView);
            break;
    }
    view->type = viewType;
}

static bool imgui_button(Renderer *renderer, Rect rect, const char *text)
{
    renderer_draw_quad(renderer, rect, Vec4(0.4f, 0.4f, 0.4f, 1.0f));
    return false;
}

static void user_interface_init(UserInterface *ui, uint32_t screenWidth, uint32_t screenHeight)
{
    layout_tree_init(&ui->layoutTree, screenWidth, screenHeight);
    struct_array_init(&ui->contextList, sizeof(void*), 32);
    struct_pool_init(&ui->contextPool, sizeof(ContextArea), 32, true);
    // right click listener for context menu
    event_manager_add_listener(g_emgr, Event_Mouse2Up, context_menu_create_proc, NULL, 100);
    ui->numFreeViews = ARRAY_COUNT(ui->viewPool);
    for(int i = 0; i < ARRAY_COUNT(ui->viewPool); i++)
    {
        ui->viewFreeList[i] = i;
    }
}

// reset immediate mode state
static void user_interface_pre_frame(UserInterface *ui)
{
    ui->cursor = CURSOR_NONE;
}

static void user_interface_free_resources(UserInterface *ui)
{
    event_manager_remove_listener(g_emgr, Event_Mouse2Up, context_menu_create_proc, NULL);
    struct_array_free_resources(&ui->contextList);
    struct_pool_free_resources(&ui->contextPool);
}

static ContextArea* user_interface_add_context(UserInterface *ui)
{
    ContextArea *ret = (ContextArea*)struct_pool_alloc(&ui->contextPool);
    if(ret == NULL)
        aike_fatal("Ran out of context entries!");
    ContextArea **listEntry = (ContextArea**)struct_array_add(&ui->contextList);
    *listEntry = ret;
    return ret;
}

static void user_interface_remove_context(UserInterface *ui, ContextArea* area)
{
    struct_pool_return(&ui->contextPool, (void*)area);

    int count = ui->contextList.count;
    bool found = false;
    for(int i = 0; i < count; i++)
    {   
        ContextArea **larea = (ContextArea**)struct_array_get(&ui->contextList, i);
        if(*larea == area)
        {
            struct_array_remove(&ui->contextList, i);    
            found = true;
            break;
        }
    }
    assert(found);
}

static ContextArea* user_interface_get_context(UserInterface *ui)
{
    int count = ui->contextList.count;
    for(int i = 0; i < count; i++)
    {
        ContextArea *ctx = *((ContextArea**)struct_array_get(&ui->contextList, i));
        // TODO: each context should have a window
        if(aike_mouse_in_window_rect(g_platform, &g_platform->mainWin, ctx->screenRect))
        {
            return ctx;
        }
    }
    return NULL;
}

static LayoutTreeNode* layout_tree_alloc_node(LayoutTree *tree)
{
    for(int i = 0; i < ARRAY_COUNT(tree->allocMap); i++)
    {
        if(tree->allocMap[i] == 0)
            continue;
        for(int j = 0; j < 8; j++)
        {
            if(((tree->allocMap[i] >> j) & 1) != 0)
            {
                tree->allocMap[i] &= ~(1<<j);
                LayoutTreeNode* ret = &tree->nodePool[i * 8 + j];
                ret->context = NULL;
                return ret;
            }
        }
    }
    return NULL;
}

static void layout_tree_init(LayoutTree* tree, uint32_t rootWidth, uint32_t rootHeight)
{
    for(int i = 0; i < ARRAY_COUNT(tree->allocMap); i++)
    {
        tree->allocMap[i] = 0xFF;
    }
    tree->rootNode = layout_tree_alloc_node(tree);
    tree->rootNode->rect = Rect(0.0f, 0.0f, rootWidth, rootHeight);
    tree->rootNode->type = LayoutTreeNodeType::Leaf;
    assert(tree->allocMap[0] == 0xFE); // check that first node was allocated
}


static void layout_tree_return_node(LayoutTree *tree, LayoutTreeNode *node)
{
    int index = ((uintptr_t)node - (uintptr_t)tree->nodePool) / sizeof(LayoutTreeNode);
    assert(index < ARRAY_COUNT(tree->nodePool)); // out of bounds
    assert((tree->allocMap[index/8] & (1<<(index&0xF))) == 0);
    tree->allocMap[index/8] |= 1<<(index&0xF);
}

#define PILLAR_WIDTH 6.0f
#define PILLAR_HWIDTH PILLAR_WIDTH/2.0f

static void layout_tree_update_node(LayoutTreeNode *node)
{
    float firstWeight = node->firstWeight;
    if(node->type == LayoutTreeNodeType::Vertical_Split)
    {
        float width0 = node->rect.width*firstWeight - PILLAR_HWIDTH;
        float width1 = node->rect.width*(1.0f-firstWeight) - PILLAR_HWIDTH;

        float originx1 = node->rect.x0 + node->rect.width*firstWeight+PILLAR_HWIDTH;

        node->children[0]->rect = Rect(node->rect.x0, node->rect.y0, width0, node->rect.height);
        node->children[1]->rect = Rect(originx1, node->rect.y0, width1, node->rect.height);

        layout_tree_update_node(node->children[0]);
        layout_tree_update_node(node->children[1]);
    }
    else if(node->type == LayoutTreeNodeType::Horizontal_Split)
    {
        float height0 = node->rect.height*firstWeight - PILLAR_HWIDTH;
        float height1 = node->rect.height*(1.0f-firstWeight) - PILLAR_HWIDTH;
        float originy1 = node->rect.y0 + node->rect.height*firstWeight + PILLAR_HWIDTH;

        node->children[0]->rect = Rect(node->rect.x0, node->rect.y0, node->rect.width, height0);
        node->children[1]->rect = Rect(node->rect.x0, originy1, node->rect.width, height1);

        layout_tree_update_node(node->children[0]);
        layout_tree_update_node(node->children[1]);
    }
    if(node->context != NULL)
    {
        assert(node->type == Leaf);
        Rect rect1 = node->context->screenRect;
        Rect rect2 = node->rect;
        node->context->screenRect = node->rect;
    }
}

static void context_area_add_option(ContextArea *ctx, const char *name, Action action, void *userData)
{
    uint32_t index = ctx->numOptions;
    strlcpy(ctx->names[index], name, ARRAY_COUNT(ctx->names[0]));
    ctx->actions[index] = action;
    ctx->userData[index] = userData;
    ctx->numOptions++; 
}

static void context_area_reset(ContextArea *ctx)
{
    ctx->numOptions = 0;
    ctx->screenRect = Rect(0, 0, 0, 0);
}

static void leaf_node_hsplit_proc(LayoutTreeNode *node)
{
    Rect rect = node->rect;
    printf("Horizontal split %f %f %f %f\n", rect.x0, rect.y0, rect.width, rect.height);
    layout_tree_split_node(&g_ui->layoutTree, node, 0.5f, true);
}

static void leaf_node_vsplit_proc(LayoutTreeNode *node)
{
    Rect rect = node->rect;
    printf("Vertical split %f %f %f %f\n", rect.x0, rect.y0, rect.width, rect.height);
    layout_tree_split_node(&g_ui->layoutTree, node, 0.5f, false);
}

static void layout_tree_register_leaf(LayoutTreeNode *node)
{
    ContextArea *ctx = user_interface_add_context(g_ui);
    context_area_reset(ctx);
    context_area_add_option(ctx, "Horizontal split", (Action)leaf_node_hsplit_proc, (void*)node);
    context_area_add_option(ctx, "Vertical split", (Action)leaf_node_vsplit_proc, (void*)node);
    // TODO: set rect and options
    ctx->screenRect = node->rect; // TODO: SCREEN rect
    node->context = ctx;

    // TODO: actual view
    node->viewState = ui_alloc_view(g_ui);
    init_view(AikeViewState::ImageView, node->viewState);
}

static void layout_tree_unregister_leaf(LayoutTreeNode *node)
{
    user_interface_remove_context(g_ui, node->context);
    node->context = NULL;

    ui_free_view(g_ui, node->viewState);
    node->viewState = NULL;
}

static void layout_tree_split_node(LayoutTree *tree, LayoutTreeNode *node, float firstWeight, bool horizontal)
{
    assert(node->type == LayoutTreeNodeType::Leaf);
    assert(firstWeight > 0.01f && firstWeight < 0.99f);

    layout_tree_unregister_leaf(node);

    node->type = horizontal ? LayoutTreeNodeType::Horizontal_Split : LayoutTreeNodeType::Vertical_Split;
    node->firstWeight = firstWeight;
    node->uiS = false;

    node->children[0] = layout_tree_alloc_node(tree);
    node->children[1] = layout_tree_alloc_node(tree);

    node->children[0]->type = LayoutTreeNodeType::Leaf;
    node->children[1]->type = LayoutTreeNodeType::Leaf;

    layout_tree_update_node(node);
    layout_tree_register_leaf(node->children[0]);
    layout_tree_register_leaf(node->children[1]);
}

//static 
static void do_layout_resize_bar(LayoutTreeNode *node, Rect rect, bool* sticky, float minX, float maxX, float minVal = 0.0f, float maxVal = 1.0f)
{
    Vec2 mousePos = g_input->mousePos;

    Vec4 pillarColor = {0.3f, 0.3f, 0.3f, 1.0f};

    // TODO layout tree should have window
    if(MOUSE1_DOWN() && aike_mouse_in_window_rect(g_platform, &g_platform->mainWin, rect))
        *sticky = true;
    else if(!MOUSE1() && *sticky)
        *sticky = false;

    bool changed = false;
    if(*sticky)
    {
        if(rect.width < rect.height)
        {
            node->firstWeight = CLAMP01((mousePos.x - minX) / (maxX - minX)); 
            changed = true;
        }
        else
        {
            node->firstWeight = CLAMP01((mousePos.y - minX) / (maxX - minX)); 
            changed = true;
        }
    }
    if(changed)
    {
        layout_tree_update_node(node);
        // update origin
        Vec2 origin;
        if(rect.width < rect.height)
        {
            origin = Vec2(node->rect.x0, node->rect.y0) 
                + Vec2(node->rect.width * node->firstWeight - PILLAR_HWIDTH, 0.0f);
        }
        else
        {
            origin = Vec2(node->rect.x0, node->rect.y0) 
                + Vec2(0.0f, node->rect.height*node->firstWeight - PILLAR_HWIDTH);
        }
        rect.x0 = origin.x;
        rect.y0 = origin.y;
    }

    renderer_draw_quad(g_renderer, rect, pillarColor);
    /*origin.x += 2.0f;
    size.x -= 4.0f;
    renderer_draw_quad(g_renderer, rect, Vec4(0.2f, 0.2f, 0.2f, 1.0f));*/
    if(aike_mouse_in_window_rect(g_platform, &g_platform->mainWin, rect))
    {
        if(rect.width < rect.height)
            g_ui->cursor = CURSOR_HORIZONTAL_ARROWS;
        else 
            g_ui->cursor = CURSOR_VERTICAL_ARROWS;
    }
}

static void layout_tree_resize(LayoutTree *tree, uint32_t width, uint32_t height)
{
    tree->rootNode->rect.width = width;
    tree->rootNode->rect.height = height;
    layout_tree_update_node(tree->rootNode);
}

static bool context_menu_create_proc(void* ptr)
{
    ContextArea* ctx = user_interface_get_context(g_ui);
    if(ctx != NULL)
    {
        ContextMenu *menu = (ContextMenu*)aike_alloc(sizeof(ContextMenu));
        context_menu_create(menu, g_input->mouseScreenPos, true);
        for(int i = 0; i < ctx->numOptions; i++)
        {
            context_menu_add_option(menu, ctx->names[i], ctx->actions[i], ctx->userData[i]);
        }
        return true;
    }
    else return false;
}

static void draw_image_view(Rect viewRect, Rect windowRect, ImageView *state)
{
    // Draw background checkerboard pattern
    //
    // TODO: find a way to do this with 1 quad?
    AikeImage *img = aike_get_dummy_image(g_aike);
    if(img != NULL)
    {
        khiter_t k = kh_get(ptr_t, img->tile_hashmap, aike_tile_hash(0, 0));
        assert(kh_exist(img->tile_hashmap, k));
        ImageTile *tile = (ImageTile*)kh_value(img->tile_hashmap, k);

        uint32_t tilesx = (int)ceilf(viewRect.width / AIKE_IMG_CHUNK_SIZE);
        uint32_t tilesy = (int)ceilf(viewRect.height / AIKE_IMG_CHUNK_SIZE);
        for(uint32_t i = 0; i < tilesx; i++)
        for(uint32_t j = 0; j < tilesy; j++)
        {
            Rect tileR(viewRect.x0 + i*AIKE_IMG_CHUNK_SIZE, viewRect.y0 + j*AIKE_IMG_CHUNK_SIZE, AIKE_IMG_CHUNK_SIZE, AIKE_IMG_CHUNK_SIZE);
            renderer_draw_tile(g_renderer, tileR, tile->glLayer);
        }
    }

    // Image controls, zooming and moving
    //
    
    bool mousehere = aike_mouse_in_window_rect(g_platform, g_renderer->curWindow, windowRect);
    if(mousehere && BOUNDKEY_DOWN(AikeInput::KB_Grab))
    {
        state->grabbing = true;
        state->grabPoint = g_input->mousePos;
        state->offsetOnGrab = state->imageSpaceCenter;
    }
    else if(state->grabbing && BOUNDKEY_UP(AikeInput::KB_Grab))
    {
        state->grabbing = false;
    }
    if(state->grabbing)
    {
        double rscale = (double)state->scale1000 / 1000.0;
        Vec2 dif = (g_input->mousePos - state->grabPoint) / rscale;
        state->imageSpaceCenter = state->offsetOnGrab - dif;
    }

    bool ctrldown = KEY(AIKE_KEY_LEFTCTRL) || KEY(AIKE_KEY_RIGHTCTRL);
    if(mousehere && ((g_input->mouseVerticalAxis < -0.001 && ctrldown)
            || BOUNDKEY_UP(AikeInput::KB_ZoomIn)))
        state->scale1000 = getNextZoom(state->scale1000);
    if(mousehere && ((g_input->mouseVerticalAxis > 0.001 && ctrldown)
            || BOUNDKEY_UP(AikeInput::KB_ZoomOut)))
        state->scale1000 = getPrevZoom(state->scale1000);

    float rscale = (float)((double)state->scale1000 / 1000.0);
    // always keep image center at the same position
    Vec2 curCenterPosition((viewRect.width / (2.0f*rscale)), (viewRect.height / (2.0f*rscale)));
    Vec2 offset = state->imageSpaceCenter - curCenterPosition;

    char buf[1024];

    Mat3 imageToView = Mat3::translateAndScale(offset*-1.0f, Vec2(rscale, rscale));
    float invScale = 1.0f/rscale;
    Mat3 viewToImage = Mat3::scaleAndTranslate(Vec2(invScale, invScale), offset);
    renderer_push_matrix(g_renderer, &imageToView);

    if(mousehere)
    {
        // TODO: should be relative to the current window
        Vec3 viewMousePos(g_input->mousePos.x, g_input->mousePos.y, 1.0f);
        viewMousePos = g_ui->windowToView * viewMousePos;
        Vec3 imageMousePos = viewToImage * viewMousePos;
        printf("mouse at %d %d (view %d %d)\n", (int)imageMousePos.x, (int)imageMousePos.y, (int)viewMousePos.x, (int)viewMousePos.y);
    }

    // draw image tiles
    //

    img = aike_get_first_image(g_aike);
    if(img == NULL)
        return;
    g_renderer->currentLayer++;

    khiter_t k;
    khash_t(ptr_t) *h = img->tile_hashmap;
    // return all tiles back to pool and clear hashmap
    for (k = kh_begin(h); k != kh_end(h); ++k)
    {
        if (kh_exist(h, k)) 
        {
            ImageTile *tile = (ImageTile*)kh_value(h, k);
            int32_t x = tile->tileX;
            int32_t y = tile->tileY;
            Vec2 min(AIKE_IMG_CHUNK_SIZE*y, AIKE_IMG_CHUNK_SIZE*x);
            Vec2 size(AIKE_IMG_CHUNK_SIZE, AIKE_IMG_CHUNK_SIZE);

            Rect tileR(min.x, min.y, size.x, size.y);
            renderer_draw_tile(g_renderer, tileR, tile->glLayer);
        }
    }
    g_renderer->currentLayer--;
    renderer_pop_matrix(g_renderer);
}

static void draw_view(Rect viewRect, AikeViewState *viewState)
{
    // TODO: this is really dirty
    // we have to do so many extra draw calls
    // just because we need GL_SCISSOR_TEST
    //    uint32_t winW = g_renderer->curWindow->screenRect.width;
    uint32_t winH = g_renderer->curWindow->height;
    renderer_render(g_renderer); // flush view

    // enable scissor test
    uint32_t glX0 = roundToInt(viewRect.x0);
    uint32_t glY0 = roundToInt(winH - viewRect.height - viewRect.y0);
    uint32_t glWidth = roundToInt(viewRect.width);
    uint32_t glHeight = roundToInt(viewRect.height);
    glScissor(glX0, glY0, glWidth, glHeight);
    glEnable(GL_SCISSOR_TEST);

    Rect localViewRect;
    localViewRect = viewRect;
    localViewRect.x0 = 0.0f;
    localViewRect.y0 = 0.0f;

    Mat3 v2w = Mat3::translate(Vec2(viewRect.x0, viewRect.y0));
    g_ui->windowToView = Mat3::translate(Vec2(-viewRect.x0, -viewRect.y0));
    renderer_push_matrix(g_renderer, &v2w);

    switch(viewState->type)
    {
        case AikeViewState::Unknown:
            break;
        case AikeViewState::ImageView:
            draw_image_view(localViewRect, viewRect, &viewState->imgView);
            break;
        default:
            fprintf(stderr, "Unknwon view! (%d)\n", viewState->type);
            break;
    }
    renderer_render(g_renderer);

    renderer_pop_matrix(g_renderer);
    glDisable(GL_SCISSOR_TEST);
}

static void layout_tree_draw(LayoutTreeNode *node)
{
    if(node->type != LayoutTreeNodeType::Leaf)
    {
        // Draw split pillar
        if(node->type == LayoutTreeNodeType::Vertical_Split)
        {
            Vec2 origin = Vec2(node->rect.x0, node->rect.y0) + Vec2(node->rect.width * node->firstWeight - PILLAR_HWIDTH, 0.0f);
            Vec2 size(PILLAR_WIDTH, node->rect.height);
            do_layout_resize_bar(node, Rect(origin, size), &node->uiS, node->rect.x0, node->rect.x0 + node->rect.width);
        }
        else if(node->type == LayoutTreeNodeType::Horizontal_Split)
        {
            Vec2 origin = Vec2(node->rect.x0, node->rect.y0) + Vec2(0.0f, node->rect.height*node->firstWeight - PILLAR_HWIDTH);
            Vec2 size(node->rect.width, PILLAR_WIDTH);
            do_layout_resize_bar(node, Rect(origin, size), &node->uiS, node->rect.y0, node->rect.y0 + node->rect.height);
        }
        else
        {
            assert(false); // not leaf and not split node
        }
        layout_tree_draw(node->children[0]);
        layout_tree_draw(node->children[1]);
    }
    else
    {
        assert(node->viewState != NULL);
        draw_view(node->rect, node->viewState);
    }
}

static void user_interface_draw(UserInterface *ui)
{
    renderer_reset(g_renderer);
    layout_tree_draw(ui->layoutTree.rootNode);

    g_renderer->currentLayer++;
#if 1
    // TODO: remove this debug crap
    Vec2 size(30.0f, 30.0f);
    Vec2 pos = g_input->mousePos - 0.5*size;

/*    float sw = g_renderer->curWindow->screenRect.width;
    float sh = g_renderer->curWindow->screenRect.height;
    Vec3 posH(pos.x, pos.y, 1.0f);
//    Mat3 mat = Mat3::proj2d(1.0f, 1.0f);
    Mat3 mat = Mat3::offsetAndScale(Vec2(-sw,-sh), 0.5f);
    posH = mat*posH;
    pos.x = posH.x; pos.y = posH.y;*/

    renderer_draw_quad(g_renderer, Rect(pos, size), Vec4(0.0f, 1.0f, 0.0f, 0.5f));
    const char *text = "TESTING TEXT... Yup heljo there. Blablblblblblblbblbllblblblblblblblbl";
    Rect rect(0.0f, 50.0f, 300.0f, 300.0f);
    g_renderer->currentLayer++;
    renderer_render_text(g_renderer, g_renderer->fontManager.cachedFonts[0], rect, text, TextAlignment_Left);
    g_renderer->currentLayer--;
    renderer_draw_quad(g_renderer, rect, Vec4(1.0f, 1.0f, 0.0f, 0.5f));
    g_renderer->currentLayer--;
#endif

    renderer_render(g_renderer);
    
    // KEEP LAST (sets cursor after whole UI is processed
    g_platform->set_cursor(g_platform, ui->cursor);
}

static void layout_tree_tests(LayoutTree *ltree)
{
    layout_tree_register_leaf(ltree->rootNode);
    layout_tree_split_node(ltree, ltree->rootNode, 0.2f, false);
    layout_tree_split_node(ltree, ltree->rootNode->children[1], 0.75f, true);
    layout_tree_split_node(ltree, ltree->rootNode->children[0], 0.75f, true);
    layout_tree_draw(ltree->rootNode);
}

static bool context_menu_close_proc(void* ptr)
{
    ContextMenu *menu = (ContextMenu*)ptr;
    context_menu_free_resources(menu);
    event_manager_remove_listener(g_emgr, Event_Mouse2Up, context_menu_close_proc, ptr);
    event_manager_remove_listener(g_emgr, Event_Mouse1Up, context_menu_leftc_close_proc, ptr);
    return true;
}

// left click while context menu is active
static bool context_menu_leftc_close_proc(void *ptr)
{
    ContextMenu *menu = (ContextMenu*)ptr;
    AikeWindow *win = &menu->window;
    if(!aike_mouse_in_window_rect(g_platform, &g_platform->mainWin, Rect(win->screenX, win->screenY, win->width, win->height)))
    {
        context_menu_close_proc(ptr);
        return true;
    }
    return false;
}

static void context_menu_create(ContextMenu *menu, Vec2 position, bool root)
{
    menu->numOptions = 0;
    menu->position = position;
    menu->rootMenu = root;
    menu->createdFrame = g_platform->frameCounter;

    g_platform->create_borderless_window(&menu->window, 200, 100);
    // TODO: get mouse position in window (might not be main window)
    //Vec2 spos = platform_window_to_screen_coord(g_platform, &g_platform->mainWin, g_input->mousePos);
    g_platform->move_window(g_platform, &menu->window, position.x, position.y);
    // TODO: find a way to do this without a global
    g_platform->menus[g_platform->numMenus++] = menu;
    event_manager_add_listener(g_emgr, Event_Mouse2Up, context_menu_close_proc, menu, 1000);
    event_manager_add_listener(g_emgr, Event_Mouse1Up, context_menu_leftc_close_proc, menu, 1000);
}

// call context_menu_close_proc instead of this
static void context_menu_free_resources(ContextMenu *menu)
{
    aike_make_window_current(&g_platform->mainWin);
    g_platform->destroy_window(g_platform, &menu->window);
    // TODO: NO? or YES??
    aike_free(menu);

    // TODO: get rid of g_platform
    bool found = false;
    for(int i = 0; i < g_platform->numMenus; i++)
    {
        if(g_platform->menus[i] == menu)
        {
            for(int j = i+1; j < g_platform->numMenus; j++)
            {
                g_platform->menus[j-1] = g_platform->menus[j];            
            }
            found = true;
            g_platform->numMenus--;
            break;
        }
    }

    if(menu->rootMenu)
        g_platform->rootMenu = NULL;

    assert(found);
}

static void context_menu_add_option(ContextMenu *menu, const char *text, Action onClick, void *onClickData)
{
    int index = menu->numOptions;
    strlcpy(menu->names[index], text, ARRAY_COUNT(menu->names[0]));
    menu->actions[index] = onClick;
    menu->actionData[index] = onClickData;
    assert(menu->names[index][strlen(text)] == 0);
    menu->numOptions++;
}

// TODO: rename to context_menu_render
static void context_menu_render(Renderer *renderer, ContextMenu *menu)
{
    renderer_reset(renderer);
    
    CachedFont *font = renderer->fontManager.cachedFonts[1];

    aike_make_window_current(&menu->window);
    g_platform->resize_window(g_platform, &menu->window, menu->window.width, menu->numOptions * (font->size+5.0f)+5.0f);
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    for(int i = 0; i < menu->numOptions; i++)
    {
        float height = menu->window.height;
        float elHeight = font->size;
        Rect trect(30.0f, i * (elHeight+5.0f), menu->window.width, elHeight);
        renderer_render_text(renderer, font, trect, menu->names[i], TextAlignment_Left);
        
        Rect mouseRect(0.0f, i * (elHeight+5.0f), menu->window.width, elHeight + 5.0f);

        if(aike_mouse_in_window_rect(g_platform, &menu->window, mouseRect))
        {
            renderer_draw_quad(renderer, mouseRect, Vec4(0.0f, 0.0f, 0.0f, 0.15f));
            if(MOUSE1_UP())
            {
                menu->actions[i](menu->actionData[i]);
                context_menu_close_proc((void*)menu);
                renderer_reset(renderer); // don't draw anything
                return;
            }
        }
    }

    renderer_render(renderer);

    g_platform->present_frame(g_platform, &menu->window);
}
