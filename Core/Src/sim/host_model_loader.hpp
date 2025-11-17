#pragma once

#include "platform/imodelloader.hpp"
#include <cstdio>

namespace mcu_game::assets
{
    class HostModelLoader : public IModelLoader
    {
    public:
        ModelLoadResult read_entire_file(const char *path, std::string &out) override;
    };
}
