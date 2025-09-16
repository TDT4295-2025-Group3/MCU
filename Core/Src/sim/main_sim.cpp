#include <iostream>

#include "game.hpp"
#include "host_platform.hpp"

int main() {
    HostInput input;
    HostRasterizer rasterizer{320, 240};
    HostTimer timer;

    Game game{rasterizer, input, timer};
    game.init();
    while (true) {
        game.tick_once();
    }

    return 0;
}
