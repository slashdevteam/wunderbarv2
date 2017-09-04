#pragma once

#include "resource.h"
#include <vector>
#include <memory>


using CurrentResources = std::vector<Resource*>;

struct Resources
{
    Resources() {};
    void registerResource(Resource* _resource)
    {
        current.emplace_back(_resource);
    }

    CurrentResources current;
};
