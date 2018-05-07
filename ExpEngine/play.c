

void play_update(TessClient *client)
{
    static V2 camRot;

    TessCamera *cam = &client->gameSystem.defaultCamera;
    TessInputSystem *input = &client->inputSystem;

    V3 forward = make_v3(0.0f, 0.0f, 0.1f);
    V3 right;
    quat_v3_mul_dir(&forward, cam->rotation, make_v3(0.0f, 0.0f, 0.01f));
    quat_v3_mul_dir(&right, cam->rotation, make_v3(0.01f, 0.0f, 0.0f));

        if(key(input, AIKE_KEY_W))
            v3_add(&cam->position, cam->position, forward);
        if(key(input, AIKE_KEY_S))
            v3_sub(&cam->position, cam->position, forward);
        if(key(input, AIKE_KEY_D))
            v3_add(&cam->position, cam->position, right);
        if(key(input, AIKE_KEY_A))
            v3_sub(&cam->position, cam->position, right);

        v2_add(&camRot, camRot, input->mouseDelta);
        camRot = make_v2(camRot.x, MAX(camRot.y, -90.0f));
        camRot = make_v2(camRot.x, MIN(camRot.y, 90.0f));

    Quat xRot, yRot;
    quat_euler_deg(&xRot, make_v3(0.0f, camRot.y, camRot.x));
    cam->rotation = xRot;

}

