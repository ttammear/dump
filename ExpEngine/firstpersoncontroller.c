

void first_person_update(FirstPersonController *c, float dt, FirstPersonControls *i) {
    Quat rot;
    quat_angle_axis(&rot, TT_RAD2DEG_F*i->yawRad, (V3){0.0f, 1.0f, 0.0f});

    bool grounded = physx_controller_is_grounded(c->phys, c->characterController);

    V3 combinedMovement = (V3){0.0f, 0.0f, 0.0f};

    V3 moveVector = (V3){0.0f, /*-9.81f*dt*/0.0f, 0.0f};
    V3 forward = (V3){0.0f, 0.0f, 4.0f};
    V3 right = (V3){4.0f, 0.0f, 0.0f};
    if(i->forward != 0) {
        v3_add(&moveVector, moveVector, forward);
    }
    if(i->backward != 0) {
        v3_sub(&moveVector, moveVector, forward);
    }
    if(i->right != 0) {
        v3_add(&moveVector, moveVector, right);
    }
    if(i->left != 0) {
        v3_sub(&moveVector, moveVector, right);
    }
    quat_v3_mul_dir(&moveVector, rot, moveVector);


    if(!grounded) {
        v3_add(&c->velocity, c->velocity, (V3){0.0f, -9.8f*dt, 0.0f});
    }
    else if(i->jump && grounded) {
        c->velocity = (V3){0.0f, 5.0f, 0.0f};
        printf("JUMP %d\r\n", i->jump);
    }


    if(grounded) {
        c->velocity.x = moveVector.x;
        c->velocity.z = moveVector.z;
        //v3_add(&c->velocity, c->velocity, moveVector);
    }
    V3 vel = v3_scale(c->velocity, dt);
    v3_add(&combinedMovement, combinedMovement, vel);
    //printf("vel %f %f %f\r\n", vel.x, vel.y, vel.z);

    physx_controller_move(c->phys, c->characterController, combinedMovement, dt);
}


 void first_person_controller_init(FirstPersonController *fpc, TessPhysicsSystem *ps, V3 pos, void *usrPtr) {
     fpc->phys = ps;
     fpc->characterController = physx_create_capsule_controller(ps, pos, usrPtr);
     fpc->velocity = (V3){0.0f, 0.0f, 0.0f};
 }

void first_person_controller_destroy(FirstPersonController *fpc) {
    physx_destroy_capsule_controller(fpc->phys, fpc->characterController);
    fpc->characterController = NULL;
}
