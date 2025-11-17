#pragma once

#include "IModelLoader.hpp"
#include <cstdio>

namespace mcu_game::assets {
    class HostModelLoader : public IModelLoader {
    public:
        ModelLoadResult read_entire_file(const char *path, std::string &out) override;

        ModelLoadResult load_model(const char *path, ModelData &outModel) override;
    };
}
