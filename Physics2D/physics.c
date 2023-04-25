#include <stdlib.h>
#include <assert.h>
#include <stdio.h>

#include "force2d.h"

F2dContext* f2d_create_context() {
    F2dContext *ret = calloc(sizeof(F2dContext), 1);
    ret->circles = calloc(sizeof(F2dCircle), 100);
    ret->boxes = calloc(sizeof(F2dBox), 100);
    ret->staticBodies = calloc(sizeof(F2dStaticBody), 200);
    ret->rigidBodies = calloc(sizeof(F2dRigidBody), 200);
    assert2(ret != NULL && ret->circles != NULL && ret->boxes != NULL && ret->staticBodies != NULL, "Allocation failed in f2d_create_context");
    return ret;
}

F2dU32x f2d_create_static_body(F2dContext *ctx, F2dShape shape, F2dV2 pos, F2dReal rot, F2dV2 scale) {
    F2dU32x id = ctx->staticBodyId++;
    F2dStaticBody *body = ctx->staticBodies + id;
    body->shapeType = shape;
    body->transform.position = pos;
    body->transform.rotation = rot;
    body->transform.scale = scale;
    switch(shape) {
        case F2D_SHAPE_CIRCLE:
            body->shapeId = ctx->circleId++;
            break;
        case F2D_SHAPE_BOX:
            body->shapeId = ctx->boxId++;
            break;
        default:
            assert2(0, "Unknown shape in f2d_create_static_body");
            break;
    }
    printf("New static body [%.2f, %.2f] angle %.2f scale [%.2f, %.2f] \n", pos.x, pos.y, rot, scale.x, scale.y);
    return id;
}

F2dU32x f2d_create_rigid_body(F2dContext *ctx, F2dShape shape, F2dV2 pos, F2dReal rot, F2dV2 scale, F2dReal mass) {
    F2dU32x id = ctx->rigidBodyId++;
    F2dRigidBody *rb = ctx->rigidBodies + id;
    rb->shapeType = shape;
    rb->transform.position = pos;
    rb->transform.rotation = rot;
    rb->transform.scale = scale;
    switch(shape) {
    case F2D_SHAPE_CIRCLE:
        rb->shapeId = ctx->circleId++;
        break;
    case F2D_SHAPE_BOX:
        rb->shapeId = ctx->boxId++;
        break;
    default:
        assert2(0, "Unknown shape in f2d_create_rigid_body");
        break;
    }
    rb->velocity = (F2dV2){F2D_ZERO, F2D_ZERO};
    rb->angularVelocity = F2D_ZERO;
    rb->mass = mass;
    printf("New rigid body [%.2f, %.2f] angle %.2f scale [%.2f, %.2f] mass %f\n", pos.x, pos.y, rot, scale.x, scale.y, mass);
    return id;
}

F2dCircle* f2d_get_circle_shape(F2dContext *ctx, F2dU32x bodyId) {
    return ctx->circles + ctx->staticBodies[bodyId].shapeId;
}

F2dBodyUpdate *f2d_get_body_updates(F2dContext *ctx, F2dU32x *count) {
    *count = 0;
    return NULL;
}
