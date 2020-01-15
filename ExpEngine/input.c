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
    if(!input->headless) {
        input->mousePrev = calc_mouse_pos(input);
    }
}

void tess_input_begin(struct TessInputSystem *input)
{
    memcpy(input->keyStatesPrev, input->keyStates, sizeof(input->keyStatesPrev));
    memcpy(input->keyStates, input->platform->keyStates, sizeof(input->keyStates));

    if(!input->headless) {
        input->mousePos = calc_mouse_pos(input);
        v2_sub(&input->mouseDelta, input->mousePos, input->mousePrev);
        input->mousePrev = input->mousePos;
        input->normMouseDelta = make_v2(input->mouseDelta.x / input->renderSystem->rtW, input->mouseDelta.y / input->renderSystem->rtH);
        input->normMousePos = make_v2(input->mousePos.x / input->renderSystem->rtW, input->mousePos.y / input->renderSystem->rtH);
        input->scroll = make_v2(input->platform->mouseHorAxis, -input->platform->mouseVerAxis);
    }
}

void tess_input_end(struct TessInputSystem *input)
{

}

// on client connect server->client
//

// per tick client->server
// frameid
// intput keys + mouselook
// tracked events (input, UI events)

// per tick server->client
// tracked transform updates
// tracked events
//      - new objects
//      - deleted objects
//      - teleported objects (if not dynamic object)

// networked input

// on client connect server sends list of tracked keys and their id
// key tracking modes
//      - track state per frame
//          for movement etc
//      - track positive edge (up->down)
//      - track negative edge (down->up)
//          for actions
//
// key state track data
//      - bitfield of all state tracked keys
//
// NOTE: might also need something like exact mouselook at edge trigger
// key edge track data
//      - 1 byte key id
//      - 1 byte flags (direction)
//
// input types
//      - keys
//      - analog
//      - spherical (mouselook)
