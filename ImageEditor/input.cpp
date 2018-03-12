
void input_init(AikeInput *input)
{
    input->mousePos = {0.0f, 0.0f};
    input->mouseVerticalAxis = 0.0f;
    input->mouseHorizontalAxis = 0.0f;

    memset(input->keyBinds, 0, sizeof(input->keyBinds));
    memset(input->keyBindStates, 0, sizeof(input->keyBindStates));
    memset(input->keyBindStatesPrev, 0, sizeof(input->keyBindStatesPrev));
    memset(input->keyStates, 0, sizeof(input->keyStates));
    memset(input->keyStatesPrev, 0, sizeof(input->keyStatesPrev));

    // TODO: load user conf
#define BINDKEY(x, n, k) (input->keyBinds[AikeInput:: x][n] = k)
    BINDKEY(KB_Grab, 0, AIKE_KEY_SPACE);
    BINDKEY(KB_Grab, 1, AIKE_BTN_MIDDLE);
    BINDKEY(KB_ZoomOut, 0, AIKE_KEY_KPMINUS);
    BINDKEY(KB_ZoomIn, 0, AIKE_KEY_KPPLUS);
#undef BINDKEY
}

void input_update(AikeInput *input, AikePlatform *platform)
{
    memcpy(input->keyBindStatesPrev, input->keyBindStates, sizeof(input->keyBindStates));
    assert(input->keyStates[0] == 0); // AIKE_KEY_RESERVED should always be 0
    bool ctrldown = KEY(AIKE_KEY_LEFTCTRL) || KEY(AIKE_KEY_RIGHTCTRL);
    bool altdown = KEY(AIKE_KEY_LEFTALT) || KEY(AIKE_KEY_RIGHTALT);
    for(int i = 0; i < AikeInput::KB_Count; i++)
    {
        bool state = false;
        for(int j = 0; j < AIKE_MAX_KEYBINDS; j++)
        {
            uint16_t rawkey = input->keyBinds[i][j];
            uint16_t key = rawkey & AIKE_KEYCODE_MASK;
            bool mod_ctrl = (rawkey & AIKE_CTRL_MASK) != 0;
            bool mod_alt = (rawkey & AIKE_ALT_MASK) != 0;
            state |= input->keyStates[key] != 0 && (!mod_ctrl || ctrldown) && (!mod_alt || altdown);        
        }
        input->keyBindStates[i] = state;
    }
    input->mouseVerticalAxis = platform->mouseVerAxis;
    input->mouseHorizontalAxis = platform->mouseHorAxis;
    
}
