
#include <chrono>

struct Physics
{
    void init();
    void deinit();
    bool simulate(bool& again);

    class btDefaultCollisionConfiguration *collisionConf;
    class btCollisionDispatcher *dispatcher;
    class btBroadphaseInterface *overlappingPairCache;
    class btSequentialImpulseConstraintSolver *solver;
    class btDiscreteDynamicsWorld *world;

    double timestep = 1.0 / 60.0;
    double offsetFromRealtime = 0.0;
    std::chrono::steady_clock::time_point lastStep;
    uint32_t stepCount = 0;
};
