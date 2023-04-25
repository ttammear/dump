#include <stdint.h>

// TODO: actually determine platform
#define F2D_PLATFORM_X86

#ifdef F2D_PLATFORM_X86
typedef float F2dReal; // floating point number
typedef uint32_t F2dU32x; // at least 32 bit but fastest
typedef int32_t F2dI32x; // at least 32 bit but fastest
typedef uint32_t F2dU32; // at least 32 bit but fastest
typedef int32_t F2dI32; // at least 32 bit but fastest
// may seem dumb but what if some day we want them to be doubles?
#define F2D_ONE 1.0f
#define F2D_ZERO 0.0f
#else
#error unknown platform
#endif

#define F2D_EXPORT extern

typedef enum F2dShape {
    F2D_SHAPE_NONE,
    F2D_SHAPE_CIRCLE,
    F2D_SHAPE_BOX,
} F2dShape;

typedef struct F2dV2 {
    float x, y;
} F2dV2;

typedef struct F2dV3 {
    float x, y, z;
} F2dV3;

/*typedef struct F2dQuat {
    float w, x, y, z;
} F2dQuat;*/

typedef struct F2dCircle {
   F2dReal radius;
} F2dCircle;

typedef struct F2dBox {
    F2dReal width;
    F2dReal height;
} F2dBox;

typedef struct F2dTransform {
    F2dV2 position;
    F2dReal rotation;
    F2dV2 scale;
} F2dTransform;

typedef struct F2dBodyUpdate {
    F2dU32x bodyId;
    F2dTransform transform;
} F2dBodyUpdate;

typedef struct F2dStaticBody {
    F2dShape shapeType;
    F2dU32x shapeId;
    F2dTransform transform;
} F2dStaticBody;

typedef struct F2dRigidBody {
    F2dShape shapeType;
    F2dU32x shapeId;
    F2dTransform transform;
    F2dV2 velocity;
    F2dReal angularVelocity;
    F2dReal mass;
} F2dRigidBody;

typedef struct F2dContext {
    F2dCircle *circles;
    F2dBox *boxes;
    F2dStaticBody *staticBodies;
    F2dRigidBody *rigidBodies;
    F2dU32x circleId;
    F2dU32x boxId;
    F2dU32x staticBodyId;
    F2dU32x rigidBodyId;
} F2dContext;


F2D_EXPORT F2dContext* f2d_create_context();
F2D_EXPORT F2dU32x f2d_create_static_body(F2dContext *ctx, F2dShape shape, F2dV2 pos, F2dReal rot, F2dV2 scale);
F2D_EXPORT F2dU32x f2d_create_rigid_body(F2dContext *ctx, F2dShape shape, F2dV2 pos, F2dReal rot, F2dV2 scale, F2dReal mass);
F2D_EXPORT F2dCircle* f2d_get_circle_shape(F2dContext *ctx, F2dU32x bodyId);
F2D_EXPORT F2dBodyUpdate *f2d_get_body_updates(F2dContext *ctx, F2dU32x *count);
