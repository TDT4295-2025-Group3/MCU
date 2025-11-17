#include "game.hpp"

#include <algorithm>
#include <cmath>
#include <iostream>

#include "constants.hpp"
#include "baked_models.hpp"
#include "game_state.hpp"
#include "collider.hpp"
#include "entities/camera.hpp"
#include "entities/platform.hpp"
#include "entities/player.hpp"
#include "entities/base_platform.hpp"
#include "entities/mushroom.hpp"
#include "game_model_loader.hpp"

static inline bool time_reached(uint32_t now, uint32_t target)
{
    // signed diff handles wraparound
    return static_cast<int32_t>(now - target) >= 0;
}

void Game::initializeHitboxDebug()
{
    uint32_t vertexCollisionId = 0xFF;
    uint32_t triangleCollisionId = 0xFF;
    if (!gameState.load_model(mcu_game::assets::baked::MeshId::Collision, vertexCollisionId, triangleCollisionId))
        return;

    for (auto &boxCollider : gameState.boxColliders)
    {
        Rasterizer::Transform transform{
            boxCollider.center.x, boxCollider.center.y, boxCollider.center.z,
            0.0f, 0.0f, 0.0f,
            boxCollider.halfExtents.x * 2.0f,
            boxCollider.halfExtents.y * 2.0f,
            boxCollider.halfExtents.z * 2.0f};

        const auto instanceResp = gameState.gfx.createInstance(static_cast<uint8_t>(vertexCollisionId),
                                                               static_cast<uint8_t>(triangleCollisionId),
                                                               transform);
        if (!instanceResp.isSuccess())
            continue;
    }
}

void Game::createEntity(mcu_game::Entity *entity)
{
    if (entity)
        entities.push_back(entity);
}

void Game::init()
{

    const auto wipeResp = gameState.gfx.wipeAll();
    if (!wipeResp.isSuccess())
    {
        std::printf("[Rasterizer] wipeAll failed (status=%u)\n", static_cast<unsigned>(wipeResp.getStatus()));
        // SevenSeg::displayNumber(29);
        // HAL_Delay(10000U);
        return;
    }

    const auto tick = gameState.timer.get_ticks_ms();

    gameState.boxColliders.clear();

    createEntity(new mcu_game::Camera({2, 2, 3}));

    createEntity(new mcu_game::BasePlatform({0.000f, 0.000f, 0.000f}));
    createEntity(new mcu_game::Mushroom({29.525f, 19.301f, -18.338f}, 0.000000f));
    createEntity(new mcu_game::Mushroom({35.755f, 24.550f, -10.860f}, 0.773123f));
    createEntity(new mcu_game::Mushroom({34.095f, 27.188f, -3.578f}, 0.773123f));
    createEntity(new mcu_game::Mushroom({28.531f, 29.826f, 1.812f}, 0.773123f));
    createEntity(new mcu_game::Mushroom({15.709f, 32.194f, 1.812f}, 0.773123f));
    createEntity(new mcu_game::Mushroom({1.263f, 35.382f, -4.608f}, 0.773123f));
    createEntity(new mcu_game::Mushroom({-12.399f, 38.604f, -12.477f}, 0.773123f));
    createEntity(new mcu_game::Platform({-12.000f, 14.000f, 1.000f}, -222.270004f));
    createEntity(new mcu_game::Platform({21.807f, 20.190f, -18.060f}, -154.498047f));
    createEntity(new mcu_game::Platform({35.740f, 24.025f, -18.060f}, -155.465286f));
    createEntity(new mcu_game::Platform({22.096f, 30.723f, 3.342f}, -155.465286f));
    createEntity(new mcu_game::Platform({8.630f, 33.655f, -1.473f}, -155.465286f));
    createEntity(new mcu_game::Platform({-5.460f, 37.080f, -8.607f}, -155.465286f));
    createEntity(new mcu_game::Platform({-18.558f, 41.062f, -14.900f}, -156.394958f));
    createEntity(new mcu_game::Platform({-4.000f, 6.400f, 15.000f}, -4.000000f));
    createEntity(new mcu_game::Platform({-7.620f, 7.980f, 18.900f}, -32.779999f));
    createEntity(new mcu_game::Platform({-9.290f, 9.560f, 13.750f}, 7.990000f));
    createEntity(new mcu_game::Platform({-12.390f, 12.070f, 10.630f}, -160.979996f));
    createEntity(new mcu_game::Platform({-8.990f, 13.670f, 5.920f}, -187.789993f));
    createEntity(new mcu_game::Platform({-12.920f, 13.820f, -5.140f}, -347.149994f));
    createEntity(new mcu_game::Platform({-11.010f, 15.780f, -11.790f}, -81.169998f));
    createEntity(new mcu_game::Platform({1.000f, 4.000f, 17.000f}, -10.000000f));
    createEntity(new mcu_game::Platform({-6.840f, 17.720f, -16.230f}, -81.169998f));
    createEntity(new mcu_game::Platform({-0.784f, 18.879f, -13.310f}, -106.449997f));
    createEntity(new mcu_game::Platform({5.730f, 20.190f, -18.060f}, -153.210007f));
    createEntity(new mcu_game::Platform({3.000f, 1.700f, 11.000f}, 15.000006f));
    createEntity(new mcu_game::Platform({14.059f, 20.190f, -18.060f}, -152.423920f));
    createEntity(new mcu_game::Player({0.000f, 1.000f, 0.000f}));

    for (auto &entity : entities)
        entity->init(gameState);

    if (showHitboxDebug)
        initializeHitboxDebug();

    next_tick_ms = tick + TICK_MS;
    next_frame_ms = tick + FRAME_MS;
}

void Game::tick_once()
{
    auto now = gameState.timer.get_ticks_ms();

    // Do catchup logic ticks
    uint32_t steps = 0;
    while (time_reached(now, next_tick_ms) && steps < MAX_CATCHUP_STEPS)
    {
        tick_logic();
        next_tick_ms += TICK_MS;
        steps++;
    }

    // Resync if we are too far behind
    if (steps == MAX_CATCHUP_STEPS && time_reached(now, next_tick_ms))
    {
        next_tick_ms = now + TICK_MS;
    }

    if (time_reached(now, next_frame_ms))
    {
        tick_graphics();
        next_frame_ms += FRAME_MS;
    }
}

void Game::tick_graphics()
{
    gameState.gfx.clear();
    for (auto &entity : entities)
        entity->render(gameState);

    gameState.gfx.end_frame();

    int yInt = static_cast<int>(std::lround(gameState.playerPosition.y));
    gameState.sevenseg.setDisplayedValue(yInt);
}

void Game::tick_logic()
{

    // Fixed dt per logic tick
    const float deltaTime = TICK_MS / 1000.0f;

    // Rigidbody inside player handles all collisions using gameState.boxColliders
    for (auto &entity : entities)
        entity->update(deltaTime, gameState);
}
