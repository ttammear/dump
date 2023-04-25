#include "force2d.h"

F2dU32x f2d_create_static_body2(F2dContext *ctx, F2dShape shape, F2dReal posX, F2dReal posY, F2dReal angle, F2dReal scaleX, F2dReal scaleY) {
    return f2d_create_static_body(ctx, shape, (F2dV2){posX, posY}, angle, (F2dV2){scaleX, scaleY});
}

F2dU32x f2d_create_rigid_body2(F2dContext *ctx, F2dShape shape, F2dReal posX, F2dReal posY, F2dReal angle, F2dReal scaleX, F2dReal scaleY, F2dReal mass) {
    return f2d_create_rigid_body(ctx, shape, (F2dV2){posX, posY}, angle, (F2dV2){scaleX, scaleY}, mass);
}

// this memory is used for stuff like pointer arguments etc
void *f2d_get_js_memory(int size) {
    return malloc(size);
}

void f2d_free_js_memory(void *ptr) {
    free(ptr);
}
