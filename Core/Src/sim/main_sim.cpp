#include <array>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>
#include <system_error>

#include "game.hpp"
#include "host_input_timer.hpp"
#include "host_rasterizer.hpp"

int main() {
    HostInput input;
    HostRasterizer rasterizer{1024, 768};
    HostTimer timer;

    std::string modelBasePath;
    bool basePathFromEnv = false;

    if (const char* envPath = std::getenv("MCU_MODEL_PATH"); envPath && envPath[0] != '\0') {
        modelBasePath = envPath;
        basePathFromEnv = true;
    } else {
        const std::filesystem::path cwd = std::filesystem::current_path();
        const std::array<std::filesystem::path, 3> candidates = {
            cwd / "models",
            cwd / "../models",
            cwd / "../../models"
        };

        for (const auto& candidate : candidates) {
            std::error_code ec;
            if (std::filesystem::is_directory(candidate, ec) && !ec) {
                const auto resolved = std::filesystem::weakly_canonical(candidate, ec);
                if (!ec) {
                    modelBasePath = resolved.string();
                    break;
                }
            }
        }
    }

    Game game{rasterizer, input, timer};
    if (!modelBasePath.empty()) {
        game.setModelBasePath(modelBasePath.c_str());
        if (basePathFromEnv) {
            std::cout << "[Model] Using MCU_MODEL_PATH=" << modelBasePath << '\n';
        } else {
            std::cout << "[Model] Using inferred model directory: " << modelBasePath << '\n';
        }
    } else {
        std::cout << "[Model] No model directory configured. Set MCU_MODEL_PATH or place models/ alongside the simulator binary." << '\n';
    }
    game.init();
    while (true) {
        game.tick_once();
    }

    return 0;
}
