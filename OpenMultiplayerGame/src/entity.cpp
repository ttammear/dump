#include "entity.h"

#include <algorithm>

void Entity::reset()
{
    for(auto component : components)
    {
        delete component;
    }
    components.clear();
    inUse = false;
    world = NULL;
}

bool Entity::removeComponent(struct Component *component)
{
    auto it = std::find(components.begin(), components.end(), component);
    if(it != components.end())
    {
        components.erase(it);
    }
    delete *it;
}
