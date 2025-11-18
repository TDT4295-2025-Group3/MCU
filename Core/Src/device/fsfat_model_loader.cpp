#include "fsfat_model_loader.hpp"
#include <limits>
#include "sd_diskio.h"

namespace mcu_game::assets {
    ModelLoadResult FsFatModelLoader::read_entire_file(const char *path, std::string &out) {
        if (!sdInitialized) {
            return ModelLoadResult::SdCardUninitialized;
        }

        FIL file{};
        const FRESULT openRes = f_open(&file, path, FA_READ);
        if (openRes != FR_OK) {
            return ModelLoadResult::FileOpenFailed;
        }

        const FSIZE_t rawSize = f_size(&file);
        out.clear();
        if (rawSize > 0) {
            if (rawSize > static_cast<FSIZE_t>(std::numeric_limits<UINT>::max())) {
                f_close(&file);
                return ModelLoadResult::DataReadFailed;
            }

            out.resize(static_cast<size_t>(rawSize));
            UINT bytesRead = 0;
            const FRESULT readRes = f_read(&file, out.data(), static_cast<UINT>(out.size()), &bytesRead);
            f_close(&file);
            if (readRes != FR_OK || bytesRead != out.size()) {
                out.clear();
                return ModelLoadResult::DataReadFailed;
            }
        } else {
            f_close(&file);
        }

        return ModelLoadResult::Ok;
    }

    FsFatModelLoader::FsFatModelLoader(SD_HandleTypeDef *sd_card_handle) {
        // Initialize SD card

        // set if SD enabled on MCU
        if (sd_card_handle->State == HAL_SD_STATE_RESET) {
            return;
        }

#if defined(SDMMC1)
        if (__HAL_RCC_SDMMC1_IS_CLK_ENABLED() == 0U) {
            return;
        }
#endif

        if (FATFS_LinkDriver(&SD_Driver, sdMountPath) != 0) {
            return;
        }

        // 2. Mount the filesystem
        const auto mountRes = f_mount(&fatFs, sdMountPath, 1);
        if (mountRes != FR_OK) {
            FATFS_UnLinkDriver(sdMountPath);
            return;
        }

        sdInitialized = true;
    }
}
