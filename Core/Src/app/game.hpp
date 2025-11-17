#pragma once
#include <memory>

#include "iinput.hpp"
#include "imodelloader.hpp"
#include "irasterizer.hpp"
#include "itimer.hpp"
#include "entities/player.hpp"
#include "entities/camera.hpp"
#include "isevenseg.hpp"

class Game
{
public:
    Game(Rasterizer::IRasterizer &gfx, IInput &in, ITimer &time, ISevenSeg &sevenseg, mcu_game::assets::IModelLoader &model_loader, bool showHitboxDebug = false)
        : gameState(in, time, sevenseg, model_loader, gfx),
          showHitboxDebug(showHitboxDebug)
    {
    }

    void init();
    void tick_once();

private:
    void tick_logic();
    void tick_graphics();
    void createEntity(mcu_game::Entity *entity);

    bool initialized = false;
    std::vector<mcu_game::Entity *> entities;
    mcu_game::GameState gameState;

    bool showHitboxDebug = false;
    void initializeHitboxDebug();

    uint32_t next_tick_ms;
    uint32_t next_frame_ms;
};
