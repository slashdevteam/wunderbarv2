#pragma once

#include "configuration.h"

using wunderbar::Configuration;

class Flash
{
public:
    Flash();

    const Configuration& getConfig() const
    {
        return config;
    }

private:
    Configuration config;
};
