#include <cstdlib>
#include <iostream>

#include "game.hpp"
#include "host_input_timer.hpp"
#include "host_rasterizer.hpp"

int main() {
    HostInput input;
    HostRasterizer rasterizer{1024, 768};
    HostTimer timer;

    Game game{rasterizer, input, timer};
    if (const char* basePath = std::getenv("MCU_MODEL_PATH")) {
        game.setModelBasePath(basePath);
    }
    game.init();
    while (true) {
        game.tick_once();
    }

    return 0;
}
