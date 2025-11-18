#pragma once

#include "math.hpp"
#include "collider.hpp"
#include <vector>

#include "iinput.hpp"
#include "imodelloader.hpp"
#include "irasterizer.hpp"
#include "isevenseg.hpp"
#include "itimer.hpp"
#include "game_model_loader.hpp"

namespace mcu_game
{
    struct GameState
    {
        bool isMenuActive{true};
        bool isEndingFishSequenceActive{false};
        Vec3 cameraForward{0.0f, 0.0f, 1.0f};
        Vec3 playerPosition{0.0f, 0.0f, 0.0f};
        Vec3 endFishPosition{0.0f, 0.0f, 0.0f};
        std::vector<BoxCollider> boxColliders;

        IInput &input;
        ITimer &timer;
        ISevenSeg &sevenseg;
        mcu_game::assets::IModelLoader &model_loader;
        Rasterizer::IRasterizer &gfx;

        bool load_model(mcu_game::assets::baked::MeshId bakedId,
                        uint32_t &outVertexId,
                        uint32_t &outTriangleId)
        {
            return createBuffersWithFallback(gfx,
                                             model_loader,
                                             bakedId,
                                             outVertexId,
                                             outTriangleId);
        }

        GameState(IInput &input_,
                  ITimer &timer_,
                  ISevenSeg &sevenseg_,
                  mcu_game::assets::IModelLoader &model_loader_,
                  Rasterizer::IRasterizer &gfx_)
            : input(input_),
              timer(timer_),
              sevenseg(sevenseg_),
              model_loader(model_loader_),
              gfx(gfx_)
        {
        }
    };
} // namespace mcu_game