#include "game.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <limits>

#include "constants.hpp"
#include "input.hpp"
#include "model_loader.hpp"

namespace {
constexpr mcu_game::Vec3 PLAYER_HALF_EXTENTS{0.5f, 0.5f, 0.5f};
constexpr float RAY_EPSILON = 1e-4f;
constexpr std::size_t MODEL_PATH_BUFFER = 128;

template <std::size_t N>
bool build_model_path(const char* basePath, const char* relativePath, char (&out)[N]) {
    if (!basePath || !relativePath) {
        return false;
    }

    const std::size_t baseLen = std::strlen(basePath);
    const bool needsSlash = (baseLen > 0) && (basePath[baseLen - 1] != '/') && (basePath[baseLen - 1] != '\\');
    const char* fmt = needsSlash ? "%s/%s" : "%s%s";
    const int written = std::snprintf(out, N, fmt, basePath, relativePath);
    if (written <= 0 || written >= static_cast<int>(N)) {
        std::printf("[Model] Path too long: base=%s rel=%s\n",
                    basePath ? basePath : "<null>",
                    relativePath ? relativePath : "<null>");
        return false;
    }

    return true;
}
}

static inline bool time_reached(uint32_t now, uint32_t target) {
    // signed diff handles wraparound
    return static_cast<int32_t>(now - target) >= 0;
}

bool Game::loadModelGeometry(const char* relativePath,
                             uint32_t& vertexId,
                             uint32_t& triangleId,
                             bool logSuccess,
                             size_t* outVertexCount,
                             size_t* outTriangleCount) {
    vertexId = 0xFF;
    triangleId = 0xFF;

    if (!modelBasePath || !relativePath) {
        return false;
    }

    char fullPath[MODEL_PATH_BUFFER];
    if (!build_model_path(modelBasePath, relativePath, fullPath)) {
        return false;
    }

    mcu_game::assets::ModelData modelData;
    const auto result = mcu_game::assets::load_model(fullPath, modelData);
    if (result != mcu_game::assets::ModelLoadResult::Ok) {
        std::printf("[Model] load_model failed for %s: %s\n", fullPath, mcu_game::assets::to_string(result));
        return false;
    }

    const size_t vertexCount = modelData.vertices.size();
    const size_t triangleCount = modelData.triangles.size();
    if (vertexCount > std::numeric_limits<uint16_t>::max() ||
        triangleCount > std::numeric_limits<uint16_t>::max()) {
        std::printf("[Model] %s exceeds rasterizer limits (%lu verts, %lu tris)\n",
                    fullPath,
                    static_cast<unsigned long>(vertexCount),
                    static_cast<unsigned long>(triangleCount));
        return false;
    }

    const auto vertResp = gfx.createVertex(modelData.vertices.data(), static_cast<uint16_t>(vertexCount));
    if (!vertResp.isSuccess()) {
        std::printf("[Model] createVertex failed for %s (status=%u)\n", fullPath, static_cast<unsigned>(vertResp.getStatus()));
        return false;
    }

    const auto triResp = gfx.createTriangle(modelData.triangles.data(), static_cast<uint16_t>(triangleCount));
    if (!triResp.isSuccess()) {
        std::printf("[Model] createTriangle failed for %s (status=%u)\n", fullPath, static_cast<unsigned>(triResp.getStatus()));
        return false;
    }

    vertexId = vertResp.getVertexId();
    triangleId = triResp.getTriangleId();

    if (outVertexCount) {
        *outVertexCount = vertexCount;
    }
    if (outTriangleCount) {
        *outTriangleCount = triangleCount;
    }

    if (logSuccess) {
        std::printf("[Model] Loaded %s (%lu verts, %lu tris)\n", fullPath,
                    static_cast<unsigned long>(vertexCount),
                    static_cast<unsigned long>(triangleCount));
    }

    return true;
}

bool Game::loadModelInstance(const char* relativePath, const Rasterizer::Transform& transform, uint32_t& instanceId) {
    if (!modelBasePath || !relativePath) {
        return false;
    }

    uint32_t vertexId = 0xFF;
    uint32_t triangleId = 0xFF;
    size_t vertexCount = 0;
    size_t triangleCount = 0;
    if (!loadModelGeometry(relativePath, vertexId, triangleId, false, &vertexCount, &triangleCount)) {
        return false;
    }

    const auto instResp = gfx.createInstance(vertexId, triangleId, transform);
    if (!instResp.isSuccess()) {
        char fullPath[MODEL_PATH_BUFFER];
        if (!build_model_path(modelBasePath, relativePath, fullPath)) {
            std::printf("[Model] createInstance failed for %s (status=%u)\n",
                        relativePath,
                        static_cast<unsigned>(instResp.getStatus()));
        } else {
            std::printf("[Model] createInstance failed for %s (status=%u)\n",
                        fullPath,
                        static_cast<unsigned>(instResp.getStatus()));
        }
        return false;
    }

    instanceId = instResp.getInstanceId();

    char fullPath[MODEL_PATH_BUFFER];
    if (!build_model_path(modelBasePath, relativePath, fullPath)) {
        std::printf("[Model] Loaded %s (%lu verts, %lu tris)\n", relativePath,
                    static_cast<unsigned long>(vertexCount),
                    static_cast<unsigned long>(triangleCount));
    } else {
        std::printf("[Model] Loaded %s (%lu verts, %lu tris)\n", fullPath,
                    static_cast<unsigned long>(vertexCount),
                    static_cast<unsigned long>(triangleCount));
    }
    return true;
}

void Game::init() {
    const auto tick = timer.get_ticks_ms();

    player.reset();
    camera.reset();
    for (auto& platform : platforms) {
        platform.instanceId = 0xFF;
    }

    instanceCubeId = 0xFF;
    instancePyrId = 0xFF;
    instancePlaneId = 0xFF;
    cubeVertexId = 0xFF;
    cubeTriangleId = 0xFF;

    const bool cubeLoaded = loadModelGeometry("cube.obj", cubeVertexId, cubeTriangleId);
    if (!cubeLoaded) {
        std::printf("[Model] Falling back to built-in cube geometry\n");
        Rasterizer::Vertex cubeVerts[8] = {
            {-0.5f, -0.5f, -0.5f, 15,  0, 15},
            { 0.5f, -0.5f, -0.5f, 15, 15, 0},
            { 0.5f,  0.5f, -0.5f,  0, 15, 15},
            {-0.5f,  0.5f, -0.5f, 15, 0,  15},
            {-0.5f, -0.5f,  0.5f, 15, 15, 0},
            { 0.5f, -0.5f,  0.5f,  0, 15, 15},
            { 0.5f,  0.5f,  0.5f, 15,  0, 15},
            {-0.5f,  0.5f,  0.5f, 15, 15,  0},
        };
        const auto createCubeVert = gfx.createVertex(cubeVerts, 8);
        if (!createCubeVert.isSuccess()) {
            std::printf("Failed to create cube vertex buffer (status=%u)\n", static_cast<unsigned>(createCubeVert.getStatus()));
            return;
        }
        cubeVertexId = createCubeVert.getVertexId();

        Rasterizer::Triangle cubeTris[12] = {
            {4,5,6}, {4,6,7},
            {1,0,3}, {1,3,2},
            {3,7,6}, {3,6,2},
            {0,1,5}, {0,5,4},
            {1,2,6}, {1,6,5},
            {0,7,3}, {0,4,7},
        };
        const auto createCubeTri = gfx.createTriangle(cubeTris, 12);
        if (!createCubeTri.isSuccess()) {
            std::printf("Failed to create cube triangle buffer (status=%u)\n", static_cast<unsigned>(createCubeTri.getStatus()));
            return;
        }
        cubeTriangleId = createCubeTri.getTriangleId();
    }

    if (cubeVertexId == 0xFF || cubeTriangleId == 0xFF) {
        std::printf("Cube geometry unavailable, aborting init\n");
        return;
    }

    Rasterizer::Transform playerTransform {
        0.0f, 0.5f, 0.0f,
        0.0f, 0.0f, 0.0f,
        1.0f, 1.0f, 1.0f
    };
    const auto cubeInstanceResp = gfx.createInstance(cubeVertexId, cubeTriangleId, playerTransform);
    if (!cubeInstanceResp.isSuccess()) {
        std::printf("Failed to create cube instance (status=%u)\n", static_cast<unsigned>(cubeInstanceResp.getStatus()));
        return;
    }
    instanceCubeId = cubeInstanceResp.getInstanceId();

    Rasterizer::Transform pyramidTransform {
        -2.0f, 0.5f, 2.0f,
        0.0f, 0.0f, 0.0f,
        1.0f, 1.0f, 1.0f
    };
    if (!loadModelInstance("pyramid.obj", pyramidTransform, instancePyrId)) {
        std::printf("[Model] Falling back to built-in pyramid geometry\n");
        Rasterizer::Vertex pyrVerts[5] = {
            {-0.5f, -0.5f, -0.5f,  0,  0,  0},
            { 0.5f, -0.5f, -0.5f, 15,  0,  0},
            { 0.5f, -0.5f,  0.5f, 15, 15,  0},
            {-0.5f, -0.5f,  0.5f,  0, 15,  0},
            { 0.0f,  0.5f,  0.0f, 15, 15, 15},
        };
        const auto createPyrVert = gfx.createVertex(pyrVerts, 5);
        if (!createPyrVert.isSuccess()) {
            std::printf("Failed to create pyramid vertex buffer (status=%u)\n", static_cast<unsigned>(createPyrVert.getStatus()));
            return;
        }

        Rasterizer::Triangle pyrTris[6] = {
            {1,0,4}, {2,1,4}, {3,2,4}, {0,3,4},
            {0,1,2}, {0,2,3},
        };
        const auto createPyrTri = gfx.createTriangle(pyrTris, 6);
        if (!createPyrTri.isSuccess()) {
            std::printf("Failed to create pyramid triangle buffer (status=%u)\n", static_cast<unsigned>(createPyrTri.getStatus()));
            return;
        }

        const auto createPyrInst = gfx.createInstance(createPyrVert.getVertexId(), createPyrTri.getTriangleId(), pyramidTransform);
        if (!createPyrInst.isSuccess()) {
            std::printf("Failed to create pyramid instance (status=%u)\n", static_cast<unsigned>(createPyrInst.getStatus()));
            return;
        }
        instancePyrId = createPyrInst.getInstanceId();
    }

    Rasterizer::Transform planeTransform {
        0.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.0f,
        1.0f, 1.0f, 1.0f
    };
    if (!loadModelInstance("plane.obj", planeTransform, instancePlaneId)) {
        std::printf("[Model] Falling back to built-in plane geometry\n");
        Rasterizer::Vertex planeVerts[4] = {
            {-4.0f, 0.0f, -4.0f, 8, 8, 8},
            { 4.0f, 0.0f, -4.0f, 8, 8, 8},
            { 4.0f, 0.0f,  4.0f, 8, 8, 8},
            {-4.0f, 0.0f,  4.0f, 8, 8, 8},
        };
        const auto createPlaneVert = gfx.createVertex(planeVerts, 4);
        if (!createPlaneVert.isSuccess()) {
            std::printf("Failed to create plane vertex buffer (status=%u)\n", static_cast<unsigned>(createPlaneVert.getStatus()));
            return;
        }

        Rasterizer::Triangle planeTris[2] = {
            {0,1,2}, {0,2,3}
        };
        const auto createPlaneTri = gfx.createTriangle(planeTris, 2);
        if (!createPlaneTri.isSuccess()) {
            std::printf("Failed to create plane triangle buffer (status=%u)\n", static_cast<unsigned>(createPlaneTri.getStatus()));
            return;
        }

        const auto createPlaneInst = gfx.createInstance(createPlaneVert.getVertexId(), createPlaneTri.getTriangleId(), planeTransform);
        if (!createPlaneInst.isSuccess()) {
            std::printf("Failed to create plane instance (status=%u)\n", static_cast<unsigned>(createPlaneInst.getStatus()));
            return;
        }
        instancePlaneId = createPlaneInst.getInstanceId();
    }

    initialize_platforms(cubeVertexId, cubeTriangleId);

    next_tick_ms = tick + TICK_MS;
    next_frame_ms = tick + FRAME_MS;
    initialized = true;
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
        gfx.updateInstance(0, 0, instanceCubeId, t);
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

    handle_player_collisions(previousPosition);

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
        platforms[i].halfExtents = defs[i].size * 0.5f;

        Rasterizer::Transform t{
            // position
            platforms[i].center.x,
            platforms[i].center.y,
            platforms[i].center.z,
            // rotation
            0.0f, 0.0f, 0.0f,
            // scale
            platforms[i].halfExtents.x * 2.0f,
            platforms[i].halfExtents.y * 2.0f,
            platforms[i].halfExtents.z * 2.0f
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

bool Game::sweep_against_box(const mcu_game::Vec3& boxCenter,
                             const mcu_game::Vec3& boxHalfExtents,
                             const mcu_game::Vec3& start,
                             const mcu_game::Vec3& delta,
                             float& outTime,
                             mcu_game::Vec3& outNormal) const {
    const mcu_game::Vec3 expandedMin = {
        boxCenter.x - boxHalfExtents.x - PLAYER_HALF_EXTENTS.x,
        boxCenter.y - boxHalfExtents.y - PLAYER_HALF_EXTENTS.y,
        boxCenter.z - boxHalfExtents.z - PLAYER_HALF_EXTENTS.z
    };
    const mcu_game::Vec3 expandedMax = {
        boxCenter.x + boxHalfExtents.x + PLAYER_HALF_EXTENTS.x,
        boxCenter.y + boxHalfExtents.y + PLAYER_HALF_EXTENTS.y,
        boxCenter.z + boxHalfExtents.z + PLAYER_HALF_EXTENTS.z
    };

    float tFirst = 0.0f;
    float tLast = 1.0f;
    mcu_game::Vec3 normal{0.0f, 0.0f, 0.0f};

    auto axisCheck = [&](float startCoord, float dir, float minCoord, float maxCoord, int axis) -> bool {
        if (std::fabs(dir) < 1e-6f) {
            if (startCoord < minCoord || startCoord > maxCoord) {
                return false;
            }
            return true;
        }

        float invDir = 1.0f / dir;
        float t1 = (minCoord - startCoord) * invDir;
        float t2 = (maxCoord - startCoord) * invDir;
        float entry = std::min(t1, t2);
        float exit = std::max(t1, t2);

        if (entry > tLast || exit < tFirst) {
            return false;
        }

        if (entry > tFirst) {
            tFirst = entry;
            normal = {0.0f, 0.0f, 0.0f};
            if (axis == 0) {
                normal.x = dir > 0.0f ? -1.0f : 1.0f;
            } else if (axis == 1) {
                normal.y = dir > 0.0f ? -1.0f : 1.0f;
            } else {
                normal.z = dir > 0.0f ? -1.0f : 1.0f;
            }
        }

        tLast = std::min(tLast, exit);
        return tFirst <= tLast;
    };

    if (!axisCheck(start.x, delta.x, expandedMin.x, expandedMax.x, 0)) return false;
    if (!axisCheck(start.y, delta.y, expandedMin.y, expandedMax.y, 1)) return false;
    if (!axisCheck(start.z, delta.z, expandedMin.z, expandedMax.z, 2)) return false;

    if (tFirst < 0.0f || tFirst > 1.0f) {
        return false;
    }

    outTime = std::max(0.0f, tFirst);
    outNormal = normal;
    return true;
}

void Game::handle_player_collisions(const mcu_game::Vec3& previousPosition) {
    const mcu_game::Vec3 prevCenter = {
        previousPosition.x,
        previousPosition.y + PLAYER_HALF_EXTENTS.y,
        previousPosition.z
    };
    const auto currentBottom = player.getPosition();
    mcu_game::Vec3 currentCenter{
        currentBottom.x,
        currentBottom.y + PLAYER_HALF_EXTENTS.y,
        currentBottom.z
    };

    mcu_game::Vec3 remainingMotion = currentCenter - prevCenter;
    mcu_game::Vec3 workingCenter = prevCenter;
    mcu_game::Vec3 workingVelocity = player.getVelocity();
    bool grounded = false;

    if (mcu_game::length_sq(remainingMotion) < 1e-8f) {
        // No displacement, but still ensure we are not below ground
        if (workingCenter.y - PLAYER_HALF_EXTENTS.y < groundCenter.y + groundHalfExtents.y) {
            workingCenter.y = groundCenter.y + groundHalfExtents.y + PLAYER_HALF_EXTENTS.y;
            grounded = true;
            if (workingVelocity.y < 0.0f) {
                workingVelocity.y = 0.0f;
            }
        }

        mcu_game::Vec3 finalBottom{
            workingCenter.x,
            workingCenter.y - PLAYER_HALF_EXTENTS.y,
            workingCenter.z
        };
        player.applyCollisionResult(finalBottom, workingVelocity, grounded);
        return;
    }

    for (int iteration = 0; iteration < 4 && mcu_game::length_sq(remainingMotion) > 1e-8f; ++iteration) {
        float bestTime = 1.0f;
        mcu_game::Vec3 bestNormal{0.0f, 0.0f, 0.0f};
        bool hitFound = false;

        auto considerCollider = [&](const mcu_game::Vec3& center, const mcu_game::Vec3& halfExtents) {
            float hitTime = 0.0f;
            mcu_game::Vec3 hitNormal{0.0f, 0.0f, 0.0f};
            if (sweep_against_box(center, halfExtents, workingCenter, remainingMotion, hitTime, hitNormal)) {
                if (hitTime < bestTime) {
                    bestTime = hitTime;
                    bestNormal = hitNormal;
                    hitFound = true;
                }
            }
        };

        for (const auto& platform : platforms) {
            if (platform.instanceId == 0xFF) continue;
            considerCollider(platform.center, platform.halfExtents);
        }
        considerCollider(groundCenter, groundHalfExtents);

        if (!hitFound) {
            workingCenter += remainingMotion;
            remainingMotion = {0.0f, 0.0f, 0.0f};
            break;
        }

        const float advance = std::max(0.0f, bestTime - RAY_EPSILON);
        workingCenter += remainingMotion * advance;

        // Remove component of velocity and remaining motion along the collision normal
        const float velAlongNormal = mcu_game::dot(workingVelocity, bestNormal);
        if (velAlongNormal < 0.0f) {
            workingVelocity -= bestNormal * velAlongNormal;
        }

        float remainingFraction = 1.0f - bestTime;
        remainingFraction = std::clamp(remainingFraction, 0.0f, 1.0f);
        remainingMotion = remainingMotion * remainingFraction;
        const float motionAlongNormal = mcu_game::dot(remainingMotion, bestNormal);
        remainingMotion -= bestNormal * motionAlongNormal;

        if (bestNormal.y > 0.5f) {
            grounded = true;
        }

        if (mcu_game::length_sq(remainingMotion) < 1e-8f) {
            remainingMotion = {0.0f, 0.0f, 0.0f};
            break;
        }
    }

    workingCenter += remainingMotion;

    // Player should never go below ground plane (y=0)
    const float groundTop = groundCenter.y + groundHalfExtents.y + PLAYER_HALF_EXTENTS.y;
    if (workingCenter.y < groundTop) {
        workingCenter.y = groundTop;
        if (workingVelocity.y < 0.0f) {
            workingVelocity.y = 0.0f;
        }
        grounded = true;
    }

    mcu_game::Vec3 finalBottom{
        workingCenter.x,
        workingCenter.y - PLAYER_HALF_EXTENTS.y,
        workingCenter.z
    };
    player.applyCollisionResult(finalBottom, workingVelocity, grounded);
}