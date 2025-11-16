#include "game.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <limits>

#include "constants.hpp"
#include "baked_models.hpp"
#include "model_loader.hpp"
#include "input.hpp"
#include "game_state.hpp"
#include "collider.hpp"
#include "entities/camera.hpp"
#include "entities/platform.hpp"
#include "entities/player.hpp"
#include "entities/base_platform.hpp"
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
    if (!createBuffersWithFallback(gfx,
                                   mcu_game::assets::baked::MeshId::Collision,
                                   vertexCollisionId,
                                   triangleCollisionId))
        return;

    for (auto &boxCollider : gameState.boxColliders)
    {
        Rasterizer::Transform transform{
            boxCollider.center.x, boxCollider.center.y, boxCollider.center.z,
            0.0f, 0.0f, 0.0f,
            boxCollider.halfExtents.x * 2.0f,
            boxCollider.halfExtents.y * 2.0f,
            boxCollider.halfExtents.z * 2.0f};

        const auto instanceResp = gfx.createInstance(static_cast<uint8_t>(vertexCollisionId),
                                                     static_cast<uint8_t>(triangleCollisionId),
                                                     transform);
        if (!instanceResp.isSuccess())
            continue;
        uint32_t instanceId = instanceResp.getInstanceId();
    }
}

void Game::createEntity(mcu_game::Entity *entity)
{
    if (entity)
        entities.push_back(entity);
}

void Game::init()
{

    const auto wipeResp = gfx.wipeAll();
    if (!wipeResp.isSuccess())
    {
        std::printf("[Rasterizer] wipeAll failed (status=%u)\n", static_cast<unsigned>(wipeResp.getStatus()));
        // SevenSeg::displayNumber(29);
        // HAL_Delay(10000U);
        return;
    }

    const auto tick = timer.get_ticks_ms();

    gameState.boxColliders.clear();

    createEntity(new mcu_game::Camera({2, 2, 3}));
    createEntity(new mcu_game::Player({0.0f, 1.0f, 0.0f}));
    createEntity(new mcu_game::Platform({3.0f, 1.0f, 8.0f}, 15.0f));
    createEntity(new mcu_game::Platform({-4.0f, 2.0f, 12.0f}, -10.0f));
    createEntity(new mcu_game::BasePlatform({-5.0f, 0.0f, 5.0f}));

    for (auto &entity : entities)
        entity->init(gfx, gameState);

    if (showHitboxDebug)
        initializeHitboxDebug();

    next_tick_ms = tick + TICK_MS;
    next_frame_ms = tick + FRAME_MS;
}

void Game::tick_once()
{
    auto now = timer.get_ticks_ms();

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
    gfx.clear();
    for (auto &entity : entities)
        entity->render(gfx);

    gfx.end_frame();
}

void Game::tick_logic()
{
    auto ks = input.poll();

    // Map keys to InputState and camera deltas
    mcu_game::InputState in{};
    // Arrow keys drive player relative to camera: up = forward (positive moveZ), right = +moveX
    in.moveZ += ks.y;
    in.moveX += ks.x;
    in.jump = ks.space;

    // WASD control camera look. Use small radians per tick.
    const float lookStep = 0.03f; // radians per logic tick
    in.lookYawDelta = lookStep * ks.cam_x;
    in.lookPitchDelta = lookStep * ks.cam_y;

    // Fixed dt per logic tick
    const float dt = TICK_MS / 1000.0f;

    // Rigidbody inside player handles all collisions using gameState.boxColliders
    for (auto &entity : entities)
        entity->update(in, dt, gameState);
}
