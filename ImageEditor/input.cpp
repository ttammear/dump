
void input_init(AikeInput *input)
{
    input->mousePos = {0.0f, 0.0f};
    input->mouseVerticalAxis = 0.0f;
    input->mouseHorizontalAxis = 0.0f;

    memset(input->keyBinds, 0, sizeof(input->keyBinds));
    memset(input->keyBindStates, 0, sizeof(input->keyBindStates));
    memset(input->keyStates, 0, sizeof(input->keyStates));
    memset(input->keyStatesPrev, 0, sizeof(input->keyStatesPrev));

    // TODO: load user conf
#define BINDKEY(x, n, k) (input->keyBinds[AikeInput:: x][n] = k)
    BINDKEY(KB_Grab, 0, AIKE_KEY_SPACE);
    BINDKEY(KB_Grab, 1, AIKE_BTN_MIDDLE);
    BINDKEY(KB_ZoomOut, 0, AIKE_KEY_MINUS); // TODO: numpad minus!
    BINDKEY(KB_ZoomIn, 0, AIKE_KEY_EQUAL); // TODO: numpad plus!
#undef BINDKEY
}

void input_update(AikeInput *input, AikePlatform *platform)
{
    memcpy(input->keyBindStatesPrev, input->keyBindStates, sizeof(input->keyBindStates));
    assert(input->keyStates[0] == 0); // AIKE_KEY_RESERVED should always be 0
    for(int i = 0; i < AikeInput::KB_Count; i++)
    {
        bool state = 0;
        for(int j = 0; j < AIKE_MAX_KEYBINDS; j++)
            state |= input->keyStates[input->keyBinds[i][j]] != 0;
        input->keyBindStates[i] = state;
    }
    input->mouseVerticalAxis = platform->mouseVerAxis;
    input->mouseHorizontalAxis = platform->mouseHorAxis;
    
}
