
void tess_input_init(struct TessInputSystem *input)
{
    memset(input->keyStates, 0, sizeof(input->keyStates));
    memset(input->keyStatesPrev, 0, sizeof(input->keyStatesPrev));
    input->mousePrev = make_v2(input->platform->mouseX, input->platform->mouseY);
}

void tess_input_begin(struct TessInputSystem *input)
{
    memcpy(input->keyStatesPrev, input->keyStates, sizeof(input->keyStatesPrev));
    memcpy(input->keyStates, input->platform->keyStates, sizeof(input->keyStates));

    V2 mouse = make_v2(input->platform->mouseX, input->platform->mouseY);
    v2_sub(&input->mouseDelta, mouse, input->mousePrev);
    input->mousePrev = mouse;
    input->mousePos = mouse;
    input->normMouseDelta = make_v2(input->mouseDelta.x / 1024.0f, input->mouseDelta.y / 768.0f);
}

void tess_input_end(struct TessInputSystem *input)
{

}

