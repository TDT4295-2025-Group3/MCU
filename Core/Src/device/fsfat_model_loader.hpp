#pragma once

#include "IModelLoader.hpp"
#include "stm32u5xx.h"
#include "ff.h"

namespace mcu_game::assets {
    class FsFatModelLoader : public IModelLoader {
    public:
        explicit FsFatModelLoader(SD_HandleTypeDef *sd_card_handle);

        ModelLoadResult read_entire_file(const char *path, std::string &out) override;

    private:
        bool sdInitialized = false;
        char sdMountPath[4] = {0}; // Store mount path like "0:/"
        FATFS fatFs; // FATFS instance
    };
}
