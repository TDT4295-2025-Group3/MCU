#include "game.hpp"

#include <cstdio>

#include "constants.hpp"

static inline bool time_reached(uint32_t now, uint32_t target) {
    // signed diff handles wraparound
    return static_cast<int32_t>(now - target) >= 0;
}

void Game::init() {
    auto tick = timer.get_ticks_ms();
    next_tick_ms = tick + TICK_MS;
    next_frame_ms = tick + FRAME_MS;
    initialized = true;

     Rasterizer::Vertex vertices[3]  = {
        {0, 0, 0, 15, 0, 0},
         {0.5, 1, 0, 0, 15, 0},
         {1, 0, 0, 0, 0, 15}
    };
    auto createVert = gfx.createVertex(vertices, 3);
    if (createVert.getStatus() != Rasterizer::StatusCode::OK) {
        printf("Failed to create vertex buffer\n");
        return;
    }
    Rasterizer::Triangle triangle = {0, 1, 2};
    auto createTri = gfx.createTriangle(&triangle, 1);

    if (createTri.getStatus() != Rasterizer::StatusCode::OK) {
        printf("Failed to create triangle buffer\n");
        return;
    }
    Rasterizer::Transform transform = {
        0, 0, 0, 0, 0, 0, 1, 1, 1
    };
    auto createInst = gfx.createInstance(createVert.getVertexId(), createTri.getTriangleId(), transform);
    if (createInst.getStatus() != Rasterizer::StatusCode::OK) {
        printf("Failed to create instance\n");
        return;
    }

    instanceId = createInst.getInstanceId();
}

void Game::tick_once() {
    // Called from sim/device in a infinite loop
    if (!initialized) init();

    auto now = timer.get_ticks_ms();

    // Do catchup logic ticks
    uint32_t steps = 0;
    while (time_reached(now, next_tick_ms) && steps < MAX_CATCHUP_STEPS) {
        tick_logic();
        next_tick_ms += TICK_MS;
        steps++;
    }

    // Resync if we are too far behind
    if (steps == MAX_CATCHUP_STEPS && time_reached(now, next_tick_ms)) {
        next_tick_ms = now + TICK_MS;
    }

    if (time_reached(now, next_frame_ms)) {
        tick_graphics();
        next_frame_ms += FRAME_MS;
    }
}

void Game::tick_graphics() {
    gfx.clear(0xFF101018);

    if (instanceId != 0xFF) {
        Rasterizer::Transform t {
            pos.x,
            pos.y,
            pos.z,
            0, 0, 0,
            1, 1, 1
        };
        gfx.updateInstance(instanceId, t);
    }

    cameraZ -= 0.005;
    Rasterizer::Transform cam {
        0, 0, cameraZ,
        0, 0, 0,
        1, 1, 1
    };
    gfx.updateCamera(cam);

    gfx.end_frame();
}

void Game::tick_logic() {
    auto ks = input.poll();
    if (ks.up) pos.y -= 0.1f;
    if (ks.down) pos.y += 0.1f;
    if (ks.left) pos.x -= 0.1f;
    if (ks.right) pos.x += 0.1;
    if (ks.a) pos.z += 0.1f;
    if (ks.b) pos.z -= 0.1f;
}