#pragma once

#include <stdint.h>
#include "ttmath.h"

typedef struct TessPhysicsSystem {
    uint32_t (*get_size)();
    void (*init)(struct TessPhysicsSystem *ps, bool debugServer);
    void (*destroy)(struct TessPhysicsSystem *ps);
} TessPhysicsSystem;

typedef void* CPxRigidActor;
typedef void* CPxController;
typedef void* CPxShape;

#ifdef __cplusplus
extern "C" {
#endif
uint32_t create_physx_physics(TessPhysicsSystem *ps);
void physics_system_init(TessPhysicsSystem *ps, bool debugServer);
void physics_system_destroy(TessPhysicsSystem *ps);
void physx_simulate(TessPhysicsSystem *ps, double dt);
CPxRigidActor physx_create_static_body_with_shapes(TessPhysicsSystem *ps, V3 pos, Quat rot, V3 scale, uint32_t shapeCount, CPxShape *shapes, void *usrPtr);
CPxRigidActor physx_create_static_body(TessPhysicsSystem *ps, V3 pos, Quat rot, void *usrPtr);
CPxRigidActor physx_create_dynamic_body(TessPhysicsSystem *ps, V3 pos, Quat rot, float mass, void *usrPtr);
void physx_destroy_body(TessPhysicsSystem *ps, CPxRigidActor actor);
CPxController physx_create_capsule_controller(TessPhysicsSystem *ps, V3 pos, void *usrPtr);
CPxShape physx_create_box_shape(TessPhysicsSystem *ps, V3 min, V3 max);
CPxShape physx_create_sphere_shape(TessPhysicsSystem *ps, V3 pos, float radius);
CPxShape physx_create_mesh_shape(TessPhysicsSystem *ps, uint32_t numVerts, V3 *verts, uint32_t numIndices, uint32_t *indices);

void physx_release_shape(TessPhysicsSystem *ps, CPxShape shape);
void physx_destroy_capsule_controller(TessPhysicsSystem *ps, CPxController c);
void physx_controller_move(TessPhysicsSystem *ps, CPxController *c, V3 disp, float dt);
bool physx_controller_is_grounded(TessPhysicsSystem *ps, CPxController *c);
void physx_add_body_to_scene(TessPhysicsSystem *ps, CPxRigidActor a);
void physx_attach_box_collider(TessPhysicsSystem *ps, CPxRigidActor a, V3 translation, Quat rot, V3 halfDimensions);


uint32_t physx_get_active_bodies(TessPhysicsSystem *ps, V3 *pos, Quat *rot, void **usrPtr, uint32_t max);
uint32_t physx_get_controllers(TessPhysicsSystem *ps, V3 *pos, void **usrPtr, uint32_t max);
#ifdef __cplusplus
}
#endif
