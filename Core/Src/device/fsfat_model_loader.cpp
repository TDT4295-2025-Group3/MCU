#include "fsfat_model_loader.hpp"
#include <limits>
#include "ff.h"

namespace mcu_game::assets {
    ModelLoadResult FsFatModelLoader::read_entire_file(const char *path, std::string &out) {
        FIL file{};
        const FRESULT openRes = f_open(&file, path, FA_READ);
        if (openRes != FR_OK) {
            std::printf("[Model] f_open failed for %s: %d\n", path, openRes);
            return ModelLoadResult::FileOpenFailed;
        }

        const FSIZE_t rawSize = f_size(&file);
        out.clear();
        if (rawSize > 0) {
            if (rawSize > static_cast<FSIZE_t>(std::numeric_limits<UINT>::max())) {
                std::printf("[Model] %s is too large (%lu bytes)\n", path, static_cast<unsigned long>(rawSize));
                f_close(&file);
                return ModelLoadResult::DataReadFailed;
            }

            out.resize(static_cast<size_t>(rawSize));
            UINT bytesRead = 0;
            const FRESULT readRes = f_read(&file, out.data(), static_cast<UINT>(out.size()), &bytesRead);
            f_close(&file);
            if (readRes != FR_OK || bytesRead != out.size()) {
                std::printf("[Model] Failed reading %s (res=%d, read=%u)\n", path, readRes, bytesRead);
                out.clear();
                return ModelLoadResult::DataReadFailed;
            }
        } else {
            f_close(&file);
        }

        return ModelLoadResult::Ok;
    }
}
