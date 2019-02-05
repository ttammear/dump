V2 calc_mouse_pos(struct TessInputSystem *input)
{
    V3 mouse = make_v3(input->platform->mouseX, input->platform->mouseY, 1.0f);
    V3 res;
    mat3_v3_mul(&res, &input->renderSystem->windowToScreen, mouse);
    return make_v2(res.x, res.y);
}

void tess_input_init(struct TessInputSystem *input)
{
    memset(input->keyStates, 0, sizeof(input->keyStates));
    memset(input->keyStatesPrev, 0, sizeof(input->keyStatesPrev));
    input->mousePrev = calc_mouse_pos(input);
}

void tess_input_begin(struct TessInputSystem *input)
{
    memcpy(input->keyStatesPrev, input->keyStates, sizeof(input->keyStatesPrev));
    memcpy(input->keyStates, input->platform->keyStates, sizeof(input->keyStates));

    input->mousePos = calc_mouse_pos(input);
    v2_sub(&input->mouseDelta, input->mousePos, input->mousePrev);
    input->mousePrev = input->mousePos;
    input->normMouseDelta = make_v2(input->mouseDelta.x / input->renderSystem->rtW, input->mouseDelta.y / input->renderSystem->rtH);
    input->normMousePos = make_v2(input->mousePos.x / input->renderSystem->rtW, input->mousePos.y / input->renderSystem->rtH);
    input->scroll = make_v2(input->platform->mouseHorAxis, -input->platform->mouseVerAxis);
}

void tess_input_end(struct TessInputSystem *input)
{

}

