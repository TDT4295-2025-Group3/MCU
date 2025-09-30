#include "game.hpp"

#include <cstdio>

#include "constants.hpp"
#include "input.hpp"

static inline bool time_reached(uint32_t now, uint32_t target) {
    // signed diff handles wraparound
    return static_cast<int32_t>(now - target) >= 0;
}

void Game::init() {
    auto tick = timer.get_ticks_ms();
    next_tick_ms = tick + TICK_MS;
    next_frame_ms = tick + FRAME_MS;
    initialized = true;

    // Reset player and camera
    player.reset();
    camera.reset();

    // Cube vertex data
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
    cubeVertexId = createCubeVert.getVertexId();

    // 12 triangles (CCW, outward)
    Rasterizer::Triangle cubeTris[12] = {
        // +Z (front)
        {4,5,6}, {4,6,7},
        // -Z (back)
        {1,0,3}, {1,3,2},
        // +Y (top)
        {3,7,6}, {3,6,2},
        // -Y (bottom)
        {0,1,5}, {0,5,4},
        // +X (right)
        {1,2,6}, {1,6,5},
        // -X (left)
        {0,7,3}, {0,4,7},
    };
    auto createCubeTri = gfx.createTriangle(cubeTris, 12);
    if (createCubeTri.getStatus() != Rasterizer::StatusCode::OK) {
        printf("Failed to create triangle buffer\n");
        return;
    }
    cubeTriangleId = createCubeTri.getTriangleId();

    // Create an instance (position=0, rotation=0, scale=1)
    Rasterizer::Transform transform = {
        0, 0.5f, 0,  // initial pos; will be updated each frame to player position
        0, 0, 0,  // rot (rad)
        1, 1, 1   // scale
    };
    auto createCubeInst = gfx.createInstance(cubeVertexId, cubeTriangleId, transform);
    if (createCubeInst.getStatus() != Rasterizer::StatusCode::OK) {
        printf("Failed to create instance\n");
        return;
    }
    instanceCubeId = createCubeInst.getInstanceId();

    // Pyramid
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
        3.0f, 0.5f, 2.5f,
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

    initialize_platforms(cubeVertexId, cubeTriangleId);
    
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
        // Update cube instance to player position
        Rasterizer::Transform t {
            player.getPosition().x, player.getPosition().y + 0.5f, player.getPosition().z,
            0.0f, 0.0f, 0.0f,
            1.0f, 1.0f, 1.0f
        };
        gfx.updateInstance(instanceCubeId, t);
    }

    // Pyramid remains static where placed in init

    // Camera locked to player using camera state
    // Convert camera position/orientation to rasterizer transform
    // We approximate by placing camera transform at camera position with rotation from yaw/pitch
    Rasterizer::Transform camT{
        camera.getPosition().x, camera.getPosition().y, camera.getPosition().z,
        // small3dlib expects rotations per-axis; we use pitch around X and yaw around Y
        camera.getPitch(), camera.getYaw(), 0.0f,
        1.0f, 1.0f, 1.0f
    };
    gfx.updateCamera(camT);

    gfx.end_frame();
}

void Game::tick_logic() {
    auto ks = input.poll();

    // Map keys to InputState and camera deltas
    mcu_game::InputState in{};
    // Arrow keys drive player relative to camera: up = forward (positive moveZ), right = +moveX
    in.moveZ += ks.up ? 1.0f : 0.0f;
    in.moveZ -= ks.down ? 1.0f : 0.0f;
    in.moveX += ks.right ? 1.0f : 0.0f;
    in.moveX -= ks.left ? 1.0f : 0.0f;
    in.jump = ks.space;

    // WASD control camera look. Use small radians per tick.
    const float lookStep = 0.03f; // radians per logic tick
    if (ks.a) in.lookYawDelta += lookStep; // A = yaw left
    if (ks.d)     in.lookYawDelta -= lookStep; // D = yaw right
    if (ks.w)     in.lookPitchDelta -= lookStep; // W = pitch up
    if (ks.s)     in.lookPitchDelta += lookStep; // S = pitch down

    // Fixed dt per logic tick
    const float dt = TICK_MS / 1000.0f;

    auto previousPosition = player.getPosition();

    // Update player with camera-relative movement first
    player.update(in, camera, dt);

    resolve_world_collisions(previousPosition);

    // Update camera using new player position so yaw/pitch orbit around the player
    camera.update(in.lookYawDelta, in.lookPitchDelta, player, dt);
}

void Game::initialize_platforms(uint32_t vertexId, uint32_t triangleId) {
    struct PlatformDef {
        mcu_game::Vec3 center;
        mcu_game::Vec3 size;
    };

    // Position and size of each platform
    constexpr PlatformDef defs[PLATFORM_COUNT] = {
        {{1.5f, 0.5f, 1.5f}, {1.5f, 0.5f, 1.5f}},
        {{-2.0f, 1.75f, 2.5f}, {1.0f, 0.5f, 1.0f}},
        {{-4.0f, 2.5f, 0.0f}, {2.0f, 0.5f, 1.5f}},
    };

    for (std::size_t i = 0; i < PLATFORM_COUNT; ++i) {
        platforms[i].center = defs[i].center;
        platforms[i].size = defs[i].size;

        Rasterizer::Transform t{
            // position
            platforms[i].center.x,
            platforms[i].center.y,
            platforms[i].center.z,
            // rotation
            0.0f, 0.0f, 0.0f,
            // scale
            platforms[i].size.x,
            platforms[i].size.y,
            platforms[i].size.z
        };

        auto inst = gfx.createInstance(vertexId, triangleId, t);
        if (inst.getStatus() == Rasterizer::StatusCode::OK) {
            platforms[i].instanceId = inst.getInstanceId();
        } else {
            platforms[i].instanceId = 0xFF;
            printf("Failed to create platform %zu instance\n", i);
        }
    }
}

void Game::resolve_world_collisions(const mcu_game::Vec3& previousPosition) {
    constexpr float PLAYER_HALF_SIZE = 0.5f;
    constexpr float PLAYER_HEIGHT = 1.0f;
    constexpr float EPSILON = 1e-4f;

    auto pos = player.getPosition();

    if (pos.y < 0.0f && player.getVelocity().y <= 0.0f) {
        player.landOn(0.0f);
        pos = player.getPosition();
    }

    float prevBottom = previousPosition.y;

    struct AABB {
        float minX, minY, minZ;
        float maxX, maxY, maxZ;
    };

    // Defines the player's bounding box
    AABB playerBox{
        pos.x - PLAYER_HALF_SIZE,
        pos.y,
        pos.z - PLAYER_HALF_SIZE,
        pos.x + PLAYER_HALF_SIZE,
        pos.y + PLAYER_HEIGHT,
        pos.z + PLAYER_HALF_SIZE
    };

    for (const auto& platform : platforms) {
        if (platform.instanceId == 0xFF) {
            continue;
        }

        const float halfX = platform.size.x * 0.5f;
        const float halfY = platform.size.y * 0.5f;
        const float halfZ = platform.size.z * 0.5f;

        const float minX = platform.center.x - halfX;
        const float maxX = platform.center.x + halfX;
        const float minY = platform.center.y - halfY;
        const float maxY = platform.center.y + halfY;
        const float minZ = platform.center.z - halfZ;
        const float maxZ = platform.center.z + halfZ;

        const bool overlapX = playerBox.maxX > minX && playerBox.minX < maxX;
        const bool overlapZ = playerBox.maxZ > minZ && playerBox.minZ < maxZ;
        if (!overlapX || !overlapZ) {
            continue;
        }

        const float velocityY = player.getVelocity().y;
        const bool wasAbove = prevBottom >= maxY - EPSILON;
        const bool nowBelowTop = playerBox.minY < maxY;

        if (velocityY <= 0.0f && wasAbove && nowBelowTop) {
            player.landOn(maxY);
            auto correctedPos = player.getPosition();
            playerBox.minY = correctedPos.y;
            playerBox.maxY = correctedPos.y + PLAYER_HEIGHT;
            prevBottom = correctedPos.y;
        }
    }
}