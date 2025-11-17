#pragma once

#include "IModelLoader.hpp"

namespace mcu_game::assets {
    class FsFatModelLoader : public IModelLoader {
    public:
        ModelLoadResult read_entire_file(const char *path, std::string &out) override;

        ModelLoadResult load_model(const char *path, ModelData &outModel) override;
    };
}
