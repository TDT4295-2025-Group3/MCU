#include "host_model_loader.hpp"

namespace mcu_game::assets {
    ModelLoadResult HostModelLoader::read_entire_file(const char *path, std::string &out) {
        std::FILE *file = std::fopen(path, "rb");
        if (file == nullptr) {
            std::printf("[Model] fopen failed for %s\n", path);
            return ModelLoadResult::FileOpenFailed;
        }
        if (std::fseek(file, 0, SEEK_END) != 0) {
            std::printf("[Model] fseek failed for %s\n", path);
            std::fclose(file);
            return ModelLoadResult::DataReadFailed;
        }
        const long size = std::ftell(file);
        if (size < 0) {
            std::printf("[Model] ftell failed for %s\n", path);
            std::fclose(file);
            return ModelLoadResult::DataReadFailed;
        }
        if (std::fseek(file, 0, SEEK_SET) != 0) {
            std::printf("[Model] rewind failed for %s\n", path);
            std::fclose(file);
            return ModelLoadResult::DataReadFailed;
        }
        out.resize(static_cast<size_t>(size));
        if (!out.empty()) {
            const size_t read = std::fread(out.data(), 1, out.size(), file);
            if (read != out.size()) {
                std::printf("[Model] fread failed for %s (read=%zu)\n", path, read);
                std::fclose(file);
                out.clear();
                return ModelLoadResult::DataReadFailed;
            }
        }
        std::fclose(file);
        return ModelLoadResult::Ok;
    }
}
