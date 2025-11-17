#pragma once

#include "math.hpp"
#include "collider.hpp"
#include <vector>

#include "iinput.hpp"
#include "imodelloader.hpp"
#include "irasterizer.hpp"
#include "isevenseg.hpp"
#include "itimer.hpp"

namespace mcu_game
{
    struct GameState
    {
        Vec3 cameraForward{0.0f, 0.0f, 1.0f};
        Vec3 playerPosition{0.0f, 0.0f, 0.0f};
        std::vector<BoxCollider> boxColliders;

        IInput &input;
        ITimer &timer;
        ISevenSeg& sevenseg;
        mcu_game::assets::IModelLoader& model_loader;
        Rasterizer::IRasterizer &gfx;

        GameState() = default;
    };
} // namespace mcu_game