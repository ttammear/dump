
void tess_input_init(struct TessInputSystem *input)
{
    memset(input->keyStates, 0, sizeof(input->keyStates));
    memset(input->keyStatesPrev, 0, sizeof(input->keyStatesPrev));
}

void tess_input_begin(struct TessInputSystem *input)
{
    memcpy(input->keyStatesPrev, input->keyStates, sizeof(input->keyStatesPrev));
    memcpy(input->keyStates, input->platform->keyStates, sizeof(input->keyStates));
}

void tess_input_end(struct TessInputSystem *input)
{

}

