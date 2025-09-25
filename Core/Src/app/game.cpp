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

    // Unit cube centered at origin
    Rasterizer::Vertex cubeVerts[8] = {
        {-0.5f, -0.5f, -0.5f, 15,  0, 15},  // 0
        { 0.5f, -0.5f, -0.5f, 15, 15, 0},   // 1
        { 0.5f,  0.5f, -0.5f,  0, 15, 15},  // 2
        {-0.5f,  0.5f, -0.5f, 15, 0,  15},  // 3
        {-0.5f, -0.5f,  0.5f, 15, 15, 0},   // 4
        { 0.5f, -0.5f,  0.5f,  0, 15, 15},  // 5
        { 0.5f,  0.5f,  0.5f, 15,  0, 15},  // 6
        {-0.5f,  0.5f,  0.5f, 15, 15,  0},  // 7
    };
    auto createCubeVert = gfx.createVertex(cubeVerts, 8);
    if (createCubeVert.getStatus() != Rasterizer::StatusCode::OK) {
        printf("Failed to create vertex buffer\n");
        return;
    }

    // 12 triangles (CCW, outward)
    Rasterizer::Triangle cubeTris[12] = {
        // +Z (front)
        {4,5,6}, {4,6,7},
        // -Z (back)
        {1,0,3}, {1,3,2},
        // +Y (top) FIXED
        {3,7,6}, {3,6,2},
        // -Y (bottom)
        {0,1,5}, {0,5,4},
        // +X (right)
        {1,2,6}, {1,6,5},
        // -X (left) FIXED
        {0,7,3}, {0,4,7},
    };
    auto createCubeTri = gfx.createTriangle(cubeTris, 12);
    if (createCubeTri.getStatus() != Rasterizer::StatusCode::OK) {
        printf("Failed to create triangle buffer\n");
        return;
    }

    // Create an instance (position=0, rotation=0, scale=1)
    Rasterizer::Transform transform = {
        0, 0.5f, 0,  // pos (lift cube above ground)
        0, 0, 0,  // rot (rad)
        1, 1, 1   // scale
    };
    auto createCubeInst = gfx.createInstance(createCubeVert.getVertexId(), createCubeTri.getTriangleId(), transform);
    if (createCubeInst.getStatus() != Rasterizer::StatusCode::OK) {
        printf("Failed to create instance\n");
        return;
    }
    instanceCubeId = createCubeInst.getInstanceId();

    // Pyramid (square base on y = -0.5, apex at y = +0.5)
    Rasterizer::Vertex pyrVerts[5] = {
        {-0.5f, -0.5f, -0.5f,  0,  0,  0},  // 0
        { 0.5f, -0.5f, -0.5f, 15,  0,  0},  // 1
        { 0.5f, -0.5f,  0.5f, 15, 15,  0},  // 2
        {-0.5f, -0.5f,  0.5f,  0, 15,  0},  // 3
        { 0.0f,  0.5f,  0.0f, 15, 15, 15},  // 4 apex
    };
    auto createPyrVert = gfx.createVertex(pyrVerts, 5);
    if (createPyrVert.getStatus() != Rasterizer::StatusCode::OK) {
        printf("Failed to create pyramid vertex buffer\n");
        return;
    }

    Rasterizer::Triangle pyrTris[6] = {
        // sides (CCW outward)
        {1,0,4}, {2,1,4}, {3,2,4}, {0,3,4},
        // base (outward -Y)
        {0,1,2}, {0,2,3},
    };
    auto createPyrTri = gfx.createTriangle(pyrTris, 6);
    if (createPyrTri.getStatus() != Rasterizer::StatusCode::OK) {
        printf("Failed to create pyramid triangle buffer\n");
        return;
    }

    Rasterizer::Transform pyrT = {
        1.2f, 0.5f, 0.0f,   // move pyramid to the right and up
        0.0f, 0.0f, 0.0f,
        1.0f, 1.0f, 1.0f
    };
    auto createPyrInst = gfx.createInstance(createPyrVert.getVertexId(), createPyrTri.getTriangleId(), pyrT);
    if (createPyrInst.getStatus() != Rasterizer::StatusCode::OK) {
        printf("Failed to create pyramid instance\n");
        return;
    }
    instancePyrId = createPyrInst.getInstanceId();

    // Ground plane: big quad on y = 0 spanning X-Z, gray color
    Rasterizer::Vertex planeVerts[4] = {
        {-4.0f, 0.0f, -4.0f, 8, 8, 8},
        { 4.0f, 0.0f, -4.0f, 8, 8, 8},
        { 4.0f, 0.0f,  4.0f, 8, 8, 8},
        {-4.0f, 0.0f,  4.0f, 8, 8, 8},
    };
    auto createPlaneVert = gfx.createVertex(planeVerts, 4);
    if (createPlaneVert.getStatus() != Rasterizer::StatusCode::OK) {
        printf("Failed to create plane vertex buffer\n");
        return;
    }

    Rasterizer::Triangle planeTris[2] = {
        {0,1,2}, {0,2,3}
    };
    auto createPlaneTri = gfx.createTriangle(planeTris, 2);
    if (createPlaneTri.getStatus() != Rasterizer::StatusCode::OK) {
        printf("Failed to create plane triangle buffer\n");
        return;
    }

    Rasterizer::Transform planeT = {
        0.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.0f,
        1.0f, 1.0f, 1.0f
    };

    auto createPlaneInst = gfx.createInstance(createPlaneVert.getVertexId(), createPlaneTri.getTriangleId(), planeT);
    if (createPlaneInst.getStatus() != Rasterizer::StatusCode::OK) {
        printf("Failed to create plane instance\n");
        return;
    }
    
    instancePlaneId = createPlaneInst.getInstanceId();
    
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

    if (instanceCubeId != 0xFF) {
        // Compute a simple rotation around Z based on time
        const float angle = (timer.get_ticks_ms() % 6000) * (2.0f * 3.1415926f / 6000.0f);

        Rasterizer::Transform t {
            pos.x, pos.y, pos.z,
            angle, angle * 0.7f, 0.0f, // rotate around X and Y
            1.0f, 1.0f, 1.0f
        };
        gfx.updateInstance(instanceCubeId, t);
    }

    if (instancePyrId != 0xFF) {
        const float t = (timer.get_ticks_ms() % 8000) * (2.0f * 3.1415926f / 8000.0f);
        Rasterizer::Transform tp {
            pos.x + 1.2f, pos.y, pos.z, // keep relative offset to cube
            0.0f, t, 0.0f,              // spin around Y
            1.0f, 1.0f, 1.0f
        };
        gfx.updateInstance(instancePyrId, tp);
    }

    Rasterizer::Transform cam {
        0, 3.0f, -6.0f,     // higher up
        -0.6f, 0, 0,        // rotate ~ -34° around X
        1, 1, 1
    };

    gfx.updateCamera(cam);

    gfx.end_frame();
}

void Game::tick_logic() {
    auto ks = input.poll();
    if (ks.up) pos.y += 0.1f;
    if (ks.down) pos.y -= 0.1f;
    if (ks.left) pos.x -= 0.1f;
    if (ks.right) pos.x += 0.1;
    if (ks.a) pos.z += 0.1f;
    if (ks.b) pos.z -= 0.1f;
}