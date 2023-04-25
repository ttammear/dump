#pragma once

#include <vector>
#include <string>

#include "transform.h"

struct Component
{
    struct Entity *entity;
    struct Transform *transform;
    const char *typeName;

    virtual void init() {}
    virtual void onUpdate(double dt) {}
    virtual void onPhysicsUpdate(double dt) {}
};

struct Entity
{
    Entity();
    void reset();

    Transform transform;
    class Mesh *mesh;
    class World *world;
    bool inUse = false;
    bool active = true;
    int id = 0;
    std::string name;

    template<typename T>
    T* addComponent()
    {
        static_assert(std::is_base_of<Component, T>::value, "Component has to be derived from Component!");
        Component *ret = (Component*)new T();
        ret->entity = this;
        ret->transform = &this->transform;
        ret->typeName = typeid(T).name();
        components.push_back(ret);
        ret->init();
        return (T*)ret;
    }

    bool removeComponent(struct Component *component);

    std::vector<struct Component*> components;
};

