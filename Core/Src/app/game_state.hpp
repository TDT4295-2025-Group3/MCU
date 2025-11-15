#pragma once

namespace mcu_game
{
    struct GameState
    {
        Camera &camera;

        Platform *platforms;
        std::size_t platformCount;

        bool &showHitboxDebug;
    };
} // namespace mcu_game