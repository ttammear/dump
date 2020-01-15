#define PHYSICS_OMIT_TYPES
#include "physics.h"
#include "PxPhysicsAPI.h"
#include "pvd/PxPvd.h"
#include "PsAtomic.h"

using namespace physx;

typedef struct TessPhysXSystem {
    TessPhysicsSystem tphysics; // MUST BE FIRST!

    PxFoundation *foundation;
    PxPhysics *physics;
    PxPvd *pvd;
    PxScene *scene;
    PxControllerManager *controllerMgr;
    PxCooking *cooking;

    PxMaterial *defaultMaterial;

    PxI32 gSharedIndex = 0;
    class ContactReportCallback *gContactReportCallback;
} TessPhysXSystem;

class ContactReportCallback : public PxSimulationEventCallback {
    void onConstraintBreak(PxConstraintInfo *constraints, PxU32 count) {
        PX_UNUSED(constraints); 
        PX_UNUSED(count); 
        printf("PhysX: constraint break\r\n");
    }
    void onWake(PxActor **actors, PxU32 count) {
        PX_UNUSED(actors);
        PX_UNUSED(count);
        printf("PhysX: onwake\r\n");
    }
    void onSleep(PxActor **actors, PxU32 count) {
        PX_UNUSED(actors);
        PX_UNUSED(count);
        printf("PhysX: onsleep\r\n");
    }
    void onTrigger(PxTriggerPair *pairs, PxU32 count) {
        PX_UNUSED(pairs);
        PX_UNUSED(count);
        printf("PhysX: ontrigger\r\n");
    }
    void onAdvance(const PxRigidBody *const*, const PxTransform *, const PxU32) {
        printf("PhysX: onadvance\r\n");
    }
    void onContact(const PxContactPairHeader &pairHeader, const PxContactPair *pairs, PxU32 nbPairs) {
        // TODO: these should not be local!
        PxVec3 gContactPositions[1000];
        PxVec3 gContactImpulses[1000];
        PxVec3 gContactVertices[1000];

        PX_UNUSED((pairHeader));
        PxContactPairPoint contactPoints[64];
        for(PxU32 i = 0; i < nbPairs; i++) {
            PxU32 contactCount = pairs[i].contactCount;
            if(contactCount) {
                pairs[i].extractContacts(&contactPoints[0], contactCount);
                PxI32 startIdx = shdfnd::atomicAdd(&physics->gSharedIndex, int32_t(contactCount));
                for(PxU32 j = 0; j < contactCount; j++) {
                    gContactPositions[startIdx+j] = contactPoints[j].position;
                    gContactImpulses[startIdx+j] = contactPoints[j].impulse;
                    gContactVertices[2*(startIdx+j)] = contactPoints[j].position;
                    gContactVertices[2*(startIdx+j) + 1] = contactPoints[j].position + contactPoints[j].impulse *0.1f;
                }
            }
        }
        printf("PhysX: oncontact\r\n");
    }
    public:
    struct TessPhysXSystem *physics; 
};

PxFilterFlags contactReportFilterShader(PxFilterObjectAttributes attributes0, PxFilterData filterData0,
	PxFilterObjectAttributes attributes1, PxFilterData filterData1,
	PxPairFlags& pairFlags, const void* constantBlock, PxU32 constantBlockSize)
{
	PX_UNUSED(attributes0);
	PX_UNUSED(attributes1);
	PX_UNUSED(filterData0);
	PX_UNUSED(filterData1);
	PX_UNUSED(constantBlockSize);
	PX_UNUSED(constantBlock);

	// all initial and persisting reports for everything, with per-point data
	pairFlags = PxPairFlag::eSOLVE_CONTACT | PxPairFlag::eDETECT_DISCRETE_CONTACT
		| PxPairFlag::eNOTIFY_TOUCH_FOUND
		| PxPairFlag::eNOTIFY_TOUCH_PERSISTS
		| PxPairFlag::eNOTIFY_CONTACT_POINTS;
	return PxFilterFlag::eDEFAULT;
}

class CallbackFinishTask : public PxLightCpuTask
{
 public:
     CallbackFinishTask() {
     }
     virtual void release() {
         PxLightCpuTask::release();
     }
     void reset() { 
     }

     void wait() {
     }

     virtual void run() {
     }

     virtual const char *getName() const {
         return "CallbackFinishTask";
     }

} callbackFinishTask;


uint32_t create_physx_physics(TessPhysicsSystem *ps) {
    if(ps != NULL) {
        ps->init = physics_system_init;
        ps->destroy = physics_system_destroy;
    }
    return sizeof(TessPhysXSystem);
}


void physics_system_init(TessPhysicsSystem *ps) {
    static PxDefaultErrorCallback gDefaultErrorCallback;
    static PxDefaultAllocator gDefaultAllocatorCallback;

    TessPhysXSystem *pxs = (TessPhysXSystem*)ps;
    pxs->gContactReportCallback = new ContactReportCallback();
    pxs->gContactReportCallback->physics = pxs;

    auto foundation = PxCreateFoundation(PX_PHYSICS_VERSION, gDefaultAllocatorCallback, gDefaultErrorCallback);
    if(foundation == nullptr) {
        fprintf(stderr, "PxCreateFoundation failed!\r\n");
        exit(-1);
    }

    bool recordMemoryAllocations = true;
    auto pvd = PxCreatePvd(*foundation);
    auto transport = PxDefaultPvdSocketTransportCreate("localhost", 5425, 100);
    //auto transport = PxDefaultPvdFileTransportCreate("out2.pxd2");
    assert(transport != NULL);
    bool pvdConnected = pvd->connect(*transport, PxPvdInstrumentationFlag::eALL);


    auto physics = PxCreatePhysics(PX_PHYSICS_VERSION, *foundation, PxTolerancesScale(), recordMemoryAllocations, pvd);
    if(physics == nullptr) {
        fprintf(stderr, "PxCreatePhysics failed!\r\n");
        exit(-1);
    }

    PxInitExtensions(*physics, pvd);
    
    PxSceneDesc sceneDesc(physics->getTolerancesScale());
    sceneDesc.gravity = PxVec3(0.0f, -9.81f, 0.0f);
    sceneDesc.cpuDispatcher = PxDefaultCpuDispatcherCreate(1);
    sceneDesc.filterShader = contactReportFilterShader;
    sceneDesc.simulationEventCallback = pxs->gContactReportCallback;
    sceneDesc.flags |= PxSceneFlag::eENABLE_ACTIVE_ACTORS;
    auto scene = physics->createScene(sceneDesc);


    //auto spvd = scene->getScenePvdClient();
    //spvd->setScenePvdFlags(PxPvdSceneFlag::eTRANSMIT_CONSTRAINTS | PxPvdSceneFlag::eTRANSMIT_SCENEQUERIES | PxPvdSceneFlag::eTRANSMIT_CONTACTS);

    auto cmgr = PxCreateControllerManager(*scene);

    auto cooking = PxCreateCooking(PX_PHYSICS_VERSION, *foundation, PxCookingParams(PxTolerancesScale()));
    if(cooking == nullptr) {
        fprintf(stderr, "Failed to initialize PhysX cooking\r\n");
        exit(-1);
    }
    
    pxs->foundation = foundation;
    pxs->physics = physics;
    pxs->pvd = pvd;
    pxs->scene = scene;
    pxs->controllerMgr = cmgr;
    pxs->cooking = cooking;

    pxs->defaultMaterial = physics->createMaterial(0.5f, 0.5f, 0.5f);

    printf("PhysX created!\r\n");
}

void physx_simulate(TessPhysicsSystem *ps, double dt) {

    TessPhysXSystem *pxs = (TessPhysXSystem*)ps;
    pxs->gSharedIndex = 0;
    auto scene = pxs->scene;

    auto cmgr = pxs->controllerMgr;
    uint32_t controllerCount = cmgr->getNbControllers();
    auto cfilter = PxControllerFilters();
    /*for(uint32_t i = 0; i < controllerCount; i++) {
        auto controller = cmgr->getController(i);
        controller->move(PxVec3(0.0f, -9.8f, 0.0f), 0.0001f, dt, cfilter);
    }*/

    scene->simulate(dt);

    const PxContactPairHeader *pHdr;
    PxU32 nbContactPairs;
    scene->fetchResultsStart(pHdr, nbContactPairs, true);
    callbackFinishTask.setContinuation(*scene->getTaskManager(), NULL);
    callbackFinishTask.reset();
    scene->processCallbacks(&callbackFinishTask);
    callbackFinishTask.removeReference();
    callbackFinishTask.wait();
    scene->fetchResultsFinish();
}

CPxRigidActor physx_create_static_body(TessPhysicsSystem *ps, V3 pos, Quat rot, void *usrPtr) {
    TessPhysXSystem *pxs = (TessPhysXSystem*)ps;
    auto p = pxs->physics;
    auto pxRot = PxQuat(rot.x, rot.y, rot.z, rot.w);
    auto pxPos = PxVec3(pos.x, pos.y, pos.z);
    auto staticActor = p->createRigidStatic(PxTransform(pxPos, pxRot));
    staticActor->userData = usrPtr;
    return staticActor;
}

CPxController physx_create_capsule_controller(TessPhysicsSystem *ps, V3 pos, void *usrPtr) {
    auto *pxs = (TessPhysXSystem*)ps;
    auto material = pxs->physics->createMaterial(0.5f, 0.5f, 0.5f); // TODO: don't use random material
    PxCapsuleControllerDesc desc;
    desc.radius = 0.5f;
    desc.height = 0.8f;
    desc.climbingMode = PxCapsuleClimbingMode::eEASY;
    // controller
    desc.stepOffset = 0.5f;
    desc.slopeLimit = 0.707; // 45 degrees in rad
    desc.volumeGrowth = 1.5f;
    desc.density = 10.0f;
    desc.contactOffset = 0.1f;
    desc.material = material;
    desc.position = PxExtendedVec3(pos.x, pos.y, pos.z);
    desc.userData = usrPtr;
    auto *c = pxs->controllerMgr->createController(desc);
    material->release();
    return c;
}


CPxShape physx_create_box_shape(TessPhysicsSystem *ps, V3 min, V3 max) {
    auto *pxs = (TessPhysXSystem*)ps;
    auto p = pxs->physics;
    V3 hext = make_v3((max.x-min.x)*0.5f, (max.y-min.y)*0.5f, (max.z-min.z)*0.5f);
    assert(hext.x > 0.0f && hext.y > 0.0f && hext.z > 0.0f);
    V3 center;
    v3_add(&center, min, hext);
    auto ret = p->createShape(PxBoxGeometry(hext.x, hext.y, hext.z), *pxs->defaultMaterial, false);
    ret->setLocalPose(PxTransform(PxVec3(center.x, center.y, center.z)));
    //printf("create physx box (%.2f %.2f %.2f) he (%.2f %.2f %.2f)\r\n", center.x, center.y, center.z, hext.x, hext.y, hext.z);
    return ret;
}

CPxShape physx_create_sphere_shape(TessPhysicsSystem *ps, V3 pos, float radius) {
    auto *pxs = (TessPhysXSystem*)ps;
    auto p = pxs->physics;
    auto ret = p->createShape(PxSphereGeometry(radius), *pxs->defaultMaterial, false);
    ret->setLocalPose(PxTransform(PxVec3(pos.x, pos.y, pos.z)));
    //printf("create physx sphere (%.2f %.2f %.2f) %f\r\n", pos.x, pos.y, pos.z, radius);
    return ret;
}

CPxShape physx_create_mesh_shape(TessPhysicsSystem *ps, uint32_t numVerts, V3 *verts, uint32_t numIndices, uint32_t *indices) {
    auto *pxs = (TessPhysXSystem*)ps;
    auto p = pxs->physics;
    static_assert(sizeof(PxVec3) == sizeof(V3) && offsetof(PxVec3, z) == offsetof(V3, z), "V3 abd PxVec3 memory layout doesn't match");
    PxTriangleMeshDesc desc;
    desc.points.count = numVerts;
    desc.points.stride = sizeof(V3);
    desc.points.data = verts;
    desc.triangles.count = numIndices;
    desc.triangles.stride = 3*sizeof(uint32_t);
    desc.triangles.data = indices;

    PxDefaultMemoryOutputStream writeBuffer;
    PxTriangleMeshCookingResult::Enum result;
    bool status = pxs->cooking->cookTriangleMesh(desc, writeBuffer, &result);
    if(!status) {
        printf("failed to create physx triangle mesh!\r\n");
        assert(0);
    }
    PxDefaultMemoryInputData readBuffer(writeBuffer.getData(), writeBuffer.getSize());
    //printf("create physx mesh with %d vertices and %d indices\r\n", numVerts, numIndices);

    auto triMesh = p->createTriangleMesh(readBuffer);
    auto ret = p->createShape(PxTriangleMeshGeometry(triMesh), *pxs->defaultMaterial, false);
    return ret;
}

void physx_release_shape(TessPhysicsSystem *ps, CPxShape shape) {
    PxShape *s = (PxShape*)shape;
    s->release();
}

void physx_controller_move(TessPhysicsSystem *ps, CPxController *c, V3 disp, float dt) {
    auto pxc = (PxController*)c;
    pxc->move(PxVec3(disp.x, disp.y, disp.z), 0.00001f, dt, PxControllerFilters());
}

bool physx_controller_is_grounded(TessPhysicsSystem *ps, CPxController *c) {
    PxControllerState state;
    auto pxc = (PxController*)c;
    pxc->getState(state);
    return state.touchedShape != nullptr;
}

void physx_destroy_capsule_controller(TessPhysicsSystem *ps, CPxController c) {
    ((PxController*)c)->release();
}

CPxRigidActor physx_create_dynamic_body(TessPhysicsSystem *ps, V3 pos, Quat rot, float mass, void *usrPtr) {
    TessPhysXSystem *pxs = (TessPhysXSystem*)ps;
    auto p = pxs->physics;
    auto pxPos = PxVec3(pos.x, pos.y, pos.z);
    auto pxRot = PxQuat(rot.x, rot.y, rot.z, rot.w);
    auto dynamicActor = p->createRigidDynamic(PxTransform(pxPos, pxRot));
    PxRigidBodyExt::setMassAndUpdateInertia(*dynamicActor, mass);
    dynamicActor->userData = usrPtr;
    return dynamicActor;
}

void physx_add_body_to_scene(TessPhysicsSystem *ps, CPxRigidActor a) {
    TessPhysXSystem *pxs = (TessPhysXSystem*)ps;
    auto p = pxs->physics;
    pxs->scene->addActor(*(PxRigidActor*)a);
}

CPxRigidActor physx_create_static_body_with_shapes(TessPhysicsSystem *ps, V3 pos, Quat rot, V3 scale, uint32_t shapeCount, CPxShape *shapes, void *usrPtr) {
    TessPhysXSystem *pxs = (TessPhysXSystem*)ps;
    auto p = pxs->physics;

    auto pxRot = PxQuat(rot.x, rot.y, rot.z, rot.w);
    auto pxPos = PxVec3(pos.x, pos.y, pos.z);
    auto pxScale = PxVec3(scale.x, scale.y, scale.z);
    auto staticActor = p->createRigidStatic(PxTransform(pxPos, pxRot));
    staticActor->userData = usrPtr;

    for(int i = 0; i < shapeCount; i++) {
        auto shape = (PxShape*)(shapes[i]);
        staticActor->attachShape(*shape);
    }
    return staticActor;
}

void physx_destroy_body(TessPhysicsSystem *ps, CPxRigidActor actor) {
    PxRigidActor *a = (PxRigidActor*)actor;
    a->release();
}

void physx_attach_box_collider(TessPhysicsSystem *ps, CPxRigidActor a, V3 translation, Quat rot, V3 halfDimensions) {
    TessPhysXSystem *pxs = (TessPhysXSystem*)ps;
    auto p = pxs->physics;
    auto material = p->createMaterial(0.5f, 0.5f, 0.5f); // TODO: don't use random material
    auto shape = PxRigidActorExt::createExclusiveShape(*(PxRigidActor*)a, PxBoxGeometry(halfDimensions.x, halfDimensions.y, halfDimensions.z), *material); 
    PxVec3 pxPos(translation.x, translation.y, translation.z);
    PxQuat pxRot(rot.x, rot.y, rot.z, rot.w);
    shape->setLocalPose(PxTransform(pxPos, pxRot));
    ((PxRigidActor*)a)->attachShape(*shape);
    shape->release();
    material->release();
}

uint32_t physx_get_active_bodies(TessPhysicsSystem *ps, V3 *pos, Quat *rot, void **usrPtr, uint32_t max) {
    TessPhysXSystem *pxs = (TessPhysXSystem*)ps;
    auto p = pxs->physics;
    PxU32 nbActiveActors;
    PxActor **activeActors = pxs->scene->getActiveActors(nbActiveActors);
    assert(nbActiveActors <= max);
    for(PxU32 i=0; i < nbActiveActors; i++) {
        assert(activeActors[i]->getType() == PxActorType::eRIGID_DYNAMIC); // TODO: cloth?
        usrPtr[i] = activeActors[i]->userData;
        auto t = ((PxRigidActor*)activeActors[i])->getGlobalPose();
        pos[i] = (V3){t.p.x, t.p.y, t.p.z};
        rot[i] = (Quat){.w = t.q.w, .x = t.q.x, .y = t.q.y, .z = t.q.z};
    }
    return nbActiveActors;
}

uint32_t physx_get_controllers(TessPhysicsSystem *ps, V3 *pos, void **usrPtr, uint32_t max) {
    TessPhysXSystem *pxs = (TessPhysXSystem*)ps;
    auto p = pxs->physics;
    auto cmgr = pxs->controllerMgr;
    PxU32 nbActiveControllers = cmgr->getNbControllers();
    assert(nbActiveControllers <= max);
    for(PxU32 i = 0; i < nbActiveControllers; i++) {
        auto controller = cmgr->getController(i);
        usrPtr[i] = controller->getUserData();
        auto pxPos = controller->getPosition();
        pos[i] = (V3){(float)pxPos.x, (float)pxPos.y, (float)pxPos.z};
    }
    return nbActiveControllers;
}

void physics_system_destroy(TessPhysicsSystem *ps) {
    TessPhysXSystem *pxs = (TessPhysXSystem*)ps;

    pxs->controllerMgr->release();
    pxs->scene->release();

    pxs->physics->release();
    if(pxs->pvd != nullptr) {
        pxs->pvd->release();
    }

    pxs->foundation->release();
    delete pxs->gContactReportCallback;
    memset(ps, 0, sizeof(*ps));
}
