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
#include "input.hpp"
#include "model_loader.hpp"
//#include "stm32u5xx_hal.h"
//#include "seven_seg_display.hpp"

namespace {
constexpr mcu_game::Vec3 PLAYER_HALF_EXTENTS{0.5f, 0.5f, 0.5f};  // Matches invisible hitbox prism
constexpr float RAY_EPSILON = 1e-4f;
constexpr std::size_t MODEL_PATH_BUFFER = 128;
constexpr float DEBUG_CUBE_DISTANCE = 18.0f;
constexpr float PLAYER_VISUAL_Y_OFFSET = -0.5f;  // Sink visual mesh slightly into collision box
constexpr float PLATFORM_VISUAL_SCALE = 1.25f;    // Inflate platform visuals without touching hitboxes

// float yaw_value = 0.0f;
uint8_t red_color = 0;
uint8_t green_color = 0;
uint8_t blue_color = 0;
// uint8_t update_color = 0;

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
        // SevenSeg::displayNumber(19);
        // HAL_Delay(10000U);
        return false;
    }

    const size_t vertexCount = modelData.vertices.size();
    const size_t triangleCount = modelData.triangles.size();
    // Removed obsolete cube instance creation
    // Streamlined player mesh initialization
    // const auto playerInstanceResp = gfx.createInstance(static_cast<uint8_t>(playerVertexId),
    //                                                    static_cast<uint8_t>(playerTriangleId),
    //                                                    {0.0f, 0.5f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f});
    const auto vertResp = gfx.createVertex(modelData.vertices.data(), static_cast<uint16_t>(vertexCount));
    if (!vertResp.isSuccess()) {
        std::printf("[Model] createVertex failed for %s (status=%u)\n", fullPath, static_cast<unsigned>(vertResp.getStatus()));
        // SevenSeg::displayNumber(21);
        // HAL_Delay(10000U);
        return false;
    }

    const auto triResp = gfx.createTriangle(modelData.triangles.data(), static_cast<uint16_t>(triangleCount));
    if (!triResp.isSuccess()) {
        std::printf("[Model] createTriangle failed for %s (status=%u)\n", fullPath, static_cast<unsigned>(triResp.getStatus()));
        // SevenSeg::displayNumber(23);
        // HAL_Delay(10000U);
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
        // SevenSeg::displayNumber(4);
        // HAL_Delay(10000U);
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
            // SevenSeg::displayNumber(25);
            // HAL_Delay(10000U);
        } else {
            std::printf("[Model] createInstance failed for %s (status=%u)\n",
                        fullPath,
                        static_cast<unsigned>(instResp.getStatus()));
            // SevenSeg::displayNumber(27);
            // HAL_Delay(10000U);
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
    const auto wipeResp = gfx.wipeAll();
    if (!wipeResp.isSuccess()) {
        std::printf("[Rasterizer] wipeAll failed (status=%u)\n", static_cast<unsigned>(wipeResp.getStatus()));
        // SevenSeg::displayNumber(29);
        // HAL_Delay(10000U);
        return;
    }

    const auto tick = timer.get_ticks_ms();

    player.reset();
    camera.reset();
    for (auto& platform : platforms) {
        platform.instanceId = 0xFF;
        platform.hitboxInstanceId = 0xFF;
    }

    playerInstanceId = 0xFF;
    instancePyrId = 0xFF;
    instancePlaneId = 0xFF;
    playerVertexId = 0xFF;
    playerTriangleId = 0xFF;
    hitboxVertexId = 0xFF;
    hitboxTriangleId = 0xFF;
    cubeVertexId = 0xFF;
    cubeTriangleId = 0xFF;
    debugCubeInstanceIds.fill(0xFF);
    hitboxDebugInstanceId = 0xFF;

    const bool cubeLoaded = loadModelGeometry("cube.obj", cubeVertexId, cubeTriangleId);
    if (!cubeLoaded) {
        std::printf("[Model] Falling back to built-in cube geometry\n");
        // SevenSeg::displayNumber(31);
        // HAL_Delay(10000U);
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
            // SevenSeg::displayNumber(33);
            // HAL_Delay(10000U);
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
            // SevenSeg::displayNumber(35);
            // HAL_Delay(10000U);
            return;
        }
        cubeTriangleId = createCubeTri.getTriangleId();
    }

    if (cubeVertexId == 0xFF || cubeTriangleId == 0xFF) {
        std::printf("Cube geometry unavailable, aborting init\n");
        // SevenSeg::displayNumber(37);
        // HAL_Delay(10000U);
        return;
    }

    {
        // Place static debug cubes around the initial camera view to verify FPGA rendering
        const mcu_game::Vec3 cameraPos = camera.getPosition();
        mcu_game::Vec3 forward = camera.getForward();
        forward.y = 0.0f;
        if (mcu_game::length_sq(forward) < 1e-6f) {
            forward = {0.0f, 0.0f, 1.0f};
        } else {
            forward = mcu_game::normalize(forward);
        }
        mcu_game::Vec3 right = camera.getRight();
        right.y = 0.0f;
        if (mcu_game::length_sq(right) < 1e-6f) {
            right = {1.0f, 0.0f, 0.0f};
        } else {
            right = mcu_game::normalize(right);
        }

        const float baseHeight = groundCenter.y + groundHalfExtents.y + PLAYER_HALF_EXTENTS.y;

        const mcu_game::Vec3 offsets[DEBUG_CUBE_COUNT] = {
            forward * DEBUG_CUBE_DISTANCE,
            forward * -DEBUG_CUBE_DISTANCE,
            right * DEBUG_CUBE_DISTANCE,
            right * -DEBUG_CUBE_DISTANCE
        };

        for (std::size_t i = 0; i < DEBUG_CUBE_COUNT; ++i) {
            mcu_game::Vec3 worldPos = cameraPos + offsets[i];
            worldPos.y = baseHeight;

            Rasterizer::Transform debugTransform{
                worldPos.x, worldPos.y, worldPos.z,
                0.0f, 0.0f, 0.0f,
                1.0f, 1.0f, 1.0f
            };

            auto instResp = gfx.createInstance(static_cast<uint8_t>(cubeVertexId),
                                               static_cast<uint8_t>(cubeTriangleId),
                                               debugTransform);
            if (!instResp.isSuccess()) {
                std::printf("[Model] Failed to create debug cube %zu (status=%u)\n",
                            i,
                            static_cast<unsigned>(instResp.getStatus()));
                // SevenSeg::displayNumber(39);
                // HAL_Delay(10000U);
                debugCubeInstanceIds[i] = 0xFF;
            } else {
                debugCubeInstanceIds[i] = instResp.getInstanceId();
            }
        }
    }

    // Hitbox prism reuses cube geometry but stays invisible (no rasterizer instance)
    hitboxVertexId = cubeVertexId;
    hitboxTriangleId = cubeTriangleId;

    const bool playerGeomLoaded = loadModelGeometry("player.obj", playerVertexId, playerTriangleId);
    if (!playerGeomLoaded) {
        std::printf("[Model] Falling back to baked player geometry\n");
        // SevenSeg::displayNumber(41);
        // HAL_Delay(10000U);
        if (!mcu_game::assets::baked::createBuffers(mcu_game::assets::baked::MeshId::Player,
                                                    gfx,
                                                    playerVertexId,
                                                    playerTriangleId)) {
            std::printf("Failed to create player geometry from baked data\n");
            // SevenSeg::displayNumber(43);
            // HAL_Delay(10000U);
            return;
        }
    }

    if (playerVertexId == 0xFF || playerTriangleId == 0xFF) {
        std::printf("Player geometry unavailable, aborting init\n");
        return;
    }

    Rasterizer::Transform playerTransform {
        0.0f, PLAYER_HALF_EXTENTS.y + PLAYER_VISUAL_Y_OFFSET, 0.0f,
        0.0f, player.getYaw(), 0.0f,
        1.0f, 1.0f, 1.0f
    };

    const auto playerInstanceResp = gfx.createInstance(static_cast<uint8_t>(playerVertexId),
                                                       static_cast<uint8_t>(playerTriangleId),
                                                       playerTransform);
    if (!playerInstanceResp.isSuccess()) {
        std::printf("Failed to create player instance (status=%u)\n",
                    static_cast<unsigned>(playerInstanceResp.getStatus()));
        // SevenSeg::displayNumber(43);
        // HAL_Delay(10000U);

        const auto fallbackInstance = gfx.createInstance(static_cast<uint8_t>(cubeVertexId),
                                                         static_cast<uint8_t>(cubeTriangleId),
                                                         playerTransform);
        if (!fallbackInstance.isSuccess()) {
            std::printf("Failed to create fallback cube instance for player (status=%u)\n",
                        static_cast<unsigned>(fallbackInstance.getStatus()));
            // SevenSeg::displayNumber(45);
            // HAL_Delay(10000U);
            return;
        }

        playerInstanceId = fallbackInstance.getInstanceId();
        playerVertexId = cubeVertexId;
        playerTriangleId = cubeTriangleId;
        std::printf("[Model] Using cube instance for player\n");
        // SevenSeg::displayNumber(47);
        // HAL_Delay(10000U);
    } else {
        playerInstanceId = playerInstanceResp.getInstanceId();
    }

    const bool platformGeomLoaded = loadModelGeometry("platform.obj", platformVertexId, platformTriangleId, false);
    if (!platformGeomLoaded) {
        std::printf("[Model] Falling back to baked platform geometry\n");
        if (!mcu_game::assets::baked::createBuffers(mcu_game::assets::baked::MeshId::Platform,
                                                    gfx,
                                                    platformVertexId,
                                                    platformTriangleId)) {
            std::printf("Failed to create platform geometry from baked data, using cube geometry instead\n");
            platformVertexId = cubeVertexId;
            platformTriangleId = cubeTriangleId;
        }
    }

    if (platformVertexId == 0xFF || platformTriangleId == 0xFF) {
        std::printf("Platform geometry unavailable, using cube geometry\n");
        platformVertexId = cubeVertexId;
        platformTriangleId = cubeTriangleId;
    }

    Rasterizer::Transform pyramidTransform {
        -2.0f, 0.5f, 2.0f,
        0.0f, 0.0f, 0.0f,
        1.0f, 1.0f, 1.0f
    };
    if (!loadModelInstance("pyramid.obj", pyramidTransform, instancePyrId)) {
        std::printf("[Model] Falling back to built-in pyramid geometry\n");
        // SevenSeg::displayNumber(49);
        // HAL_Delay(10000U);
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
        // SevenSeg::displayNumber(51);
        // HAL_Delay(10000U);
        Rasterizer::Vertex planeVerts[4] = {
            {-0.5f, 0.0f, -0.5f, 8, 8, 8},
            { 0.5f, 0.0f, -0.5f, 8, 8, 8},
            { 0.5f, 0.0f,  0.5f, 8, 8, 8},
            {-0.5f, 0.0f,  0.5f, 8, 8, 8},
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

    initialize_platforms();

    const Rasterizer::Transform initialCameraTransform{
        camera.getPosition().x,
        camera.getPosition().y,
        camera.getPosition().z,
        camera.getPitch(),
        camera.getYaw(),
        0.0f,
        1.0f, 1.0f, 1.0f
    };
    const auto cameraResp = gfx.updateCamera(0, 0, 0, initialCameraTransform);
    if (!cameraResp.isSuccess()) {
        std::printf("[Rasterizer] updateCamera failed (status=%u)\n", static_cast<unsigned>(cameraResp.getStatus()));
        return;
    }

    next_tick_ms = tick + TICK_MS;
    next_frame_ms = tick + FRAME_MS;
    initialized = true;
    updateHitboxDebugInstance();
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

    if (playerInstanceId != 0xFF) {
        // Update player render instance to follow current player position
        Rasterizer::Transform t {
            player.getPosition().x,
            player.getPosition().y + PLAYER_HALF_EXTENTS.y + PLAYER_VISUAL_Y_OFFSET,
            player.getPosition().z,
            0.0f, player.getYaw(), 0.0f,
            1.0f, 1.0f, 1.0f
        };
        gfx.updateInstance(static_cast<uint8_t>(playerVertexId),
                           static_cast<uint8_t>(playerTriangleId),
                           static_cast<uint8_t>(playerInstanceId),
                           t);
    }

    updateHitboxDebugInstance();

    // Pyramid remains static where placed in init

    // Camera locked to player using camera state
    // Convert camera position/orientation to rasterizer transform
    // We approximate by placing camera transform at camera position with rotation from yaw/pitch
    Rasterizer::Transform camT{
        camera.getPosition().x, camera.getPosition().y, camera.getPosition().z,
        // small3dlib expects rotations per-axis; we use pitch around X and yaw around Y
        camera.getPitch(), camera.getYaw(), 0.0f,  // add yaw_value for debugging here
        1.0f, 1.0f, 1.0f
    };

    gfx.updateCamera(red_color, green_color, blue_color, camT);

    // if (update_color % 256 == 0) {
    //     red_color = (red_color + 1) % 16;
    //     green_color = (green_color + 2) % 16;
    //     blue_color = (blue_color + 3) % 16;
    // }
    // update_color++;
    

    // yaw_value += 2 * 3.14159265f / (10*60.0f);

    gfx.end_frame();
}

// Only for debug hitbox visualization
void Game::updateHitboxDebugInstance() {
    if (!initialized) {
        return;
    }

    if (showHitboxDebug) {
        if (hitboxDebugInstanceId == 0xFF && hitboxVertexId != 0xFF && hitboxTriangleId != 0xFF) {
            Rasterizer::Transform t{
                player.getPosition().x,
                player.getPosition().y + PLAYER_HALF_EXTENTS.y,
                player.getPosition().z,
                0.0f, 0.0f, 0.0f,
                PLAYER_HALF_EXTENTS.x * 2.0f,
                PLAYER_HALF_EXTENTS.y * 2.0f,
                PLAYER_HALF_EXTENTS.z * 2.0f
            };

            auto instResp = gfx.createInstance(static_cast<uint8_t>(hitboxVertexId),
                                               static_cast<uint8_t>(hitboxTriangleId),
                                               t);
            if (instResp.isSuccess()) {
                hitboxDebugInstanceId = instResp.getInstanceId();
            } else {
                std::printf("[Model] Failed to create hitbox debug instance (status=%u)\n",
                            static_cast<unsigned>(instResp.getStatus()));
                hitboxDebugInstanceId = 0xFF;
            }
        }

        if (hitboxDebugInstanceId != 0xFF) {
            Rasterizer::Transform t{
                player.getPosition().x,
                player.getPosition().y + PLAYER_HALF_EXTENTS.y,
                player.getPosition().z,
                0.0f, 0.0f, 0.0f,
                PLAYER_HALF_EXTENTS.x * 2.0f,
                PLAYER_HALF_EXTENTS.y * 2.0f,
                PLAYER_HALF_EXTENTS.z * 2.0f
            };

            gfx.updateInstance(static_cast<uint8_t>(hitboxVertexId),
                               static_cast<uint8_t>(hitboxTriangleId),
                               static_cast<uint8_t>(hitboxDebugInstanceId),
                               t);
        }

        if (hitboxVertexId != 0xFF && hitboxTriangleId != 0xFF) {
            for (auto& platform : platforms) {
            Rasterizer::Transform hitboxTransform{
                platform.center.x,
                platform.center.y,
                platform.center.z,
                0.0f, 0.0f, 0.0f,
                platform.halfExtents.x * 2.0f,
                platform.halfExtents.y * 2.0f,
                platform.halfExtents.z * 2.0f
            };

            if (platform.hitboxInstanceId == 0xFF) {
                auto instResp = gfx.createInstance(static_cast<uint8_t>(hitboxVertexId),
                                                   static_cast<uint8_t>(hitboxTriangleId),
                                                   hitboxTransform);
                if (instResp.isSuccess()) {
                    platform.hitboxInstanceId = instResp.getInstanceId();
                } else {
                    std::printf("[Model] Failed to create platform hitbox instance (status=%u)\n",
                                static_cast<unsigned>(instResp.getStatus()));
                    platform.hitboxInstanceId = 0xFF;
                }
            }

            if (platform.hitboxInstanceId != 0xFF) {
                gfx.updateInstance(static_cast<uint8_t>(hitboxVertexId),
                                   static_cast<uint8_t>(hitboxTriangleId),
                                   static_cast<uint8_t>(platform.hitboxInstanceId),
                                   hitboxTransform);
            }
        }
        }
    } else {
        if (hitboxDebugInstanceId != 0xFF) {
            Rasterizer::Transform hide{
                player.getPosition().x,
                player.getPosition().y + PLAYER_HALF_EXTENTS.y - 100.0f,
                player.getPosition().z,
                0.0f, 0.0f, 0.0f,
                0.0f, 0.0f, 0.0f
            };

            gfx.updateInstance(static_cast<uint8_t>(hitboxVertexId),
                               static_cast<uint8_t>(hitboxTriangleId),
                               static_cast<uint8_t>(hitboxDebugInstanceId),
                               hide);
        }

        if (hitboxVertexId != 0xFF && hitboxTriangleId != 0xFF) {
            for (auto& platform : platforms) {
                if (platform.hitboxInstanceId == 0xFF) {
                    continue;
                }

                Rasterizer::Transform hide{
                    platform.center.x,
                    platform.center.y - 100.0f,
                    platform.center.z,
                    0.0f, 0.0f, 0.0f,
                    0.0f, 0.0f, 0.0f
                };

                gfx.updateInstance(static_cast<uint8_t>(hitboxVertexId),
                                   static_cast<uint8_t>(hitboxTriangleId),
                                   static_cast<uint8_t>(platform.hitboxInstanceId),
                                   hide);
            }
        }
    }
}

void Game::tick_logic() {
    auto ks = input.poll();

    // Map keys to InputState and camera deltas
    mcu_game::InputState in{};
    // Arrow keys drive player relative to camera forward/right
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

void Game::initialize_platforms() {
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

    const bool platformMeshReady = (platformVertexId != cubeVertexId) || (platformTriangleId != cubeTriangleId);
    bool usePlatformMesh = platformMeshReady;
    mcu_game::Vec3 meshCenter{0.0f, 0.0f, 0.0f};
    mcu_game::Vec3 meshHalfExtents{0.5f, 0.5f, 0.5f};

    if (usePlatformMesh) {
        const auto& mesh = mcu_game::assets::baked::getMesh(mcu_game::assets::baked::MeshId::Platform);
        if (!mesh.vertices || mesh.vertexCount == 0) {
            std::printf("[Platform] Baked mesh data missing, reverting to cube geometry\n");
            usePlatformMesh = false;
        } else {
            mcu_game::Vec3 min{mesh.vertices[0].x, mesh.vertices[0].y, mesh.vertices[0].z};
            mcu_game::Vec3 max = min;
            for (std::size_t v = 1; v < mesh.vertexCount; ++v) {
                const auto& vert = mesh.vertices[v];
                min.x = std::min(min.x, vert.x);
                min.y = std::min(min.y, vert.y);
                min.z = std::min(min.z, vert.z);
                max.x = std::max(max.x, vert.x);
                max.y = std::max(max.y, vert.y);
                max.z = std::max(max.z, vert.z);
            }

            meshCenter = (min + max) * 0.5f;
            meshHalfExtents = (max - min) * 0.5f;

            if (meshHalfExtents.x <= 1e-4f || meshHalfExtents.y <= 1e-4f || meshHalfExtents.z <= 1e-4f) {
                std::printf("[Platform] Baked mesh bounds degenerate, reverting to cube geometry\n");
                usePlatformMesh = false;
            }
        }
    }

    const auto safeScale = [](float targetHalf, float baseHalf) {
        constexpr float EPS = 1e-4f;
        if (baseHalf <= EPS) {
            return 1.0f;
        }
        return targetHalf / baseHalf;
    };

    for (std::size_t i = 0; i < PLATFORM_COUNT; ++i) {
        platforms[i].center = defs[i].center;
        platforms[i].halfExtents = defs[i].size * 0.5f;

        if (usePlatformMesh) {
            const float baseScaleX = safeScale(platforms[i].halfExtents.x, meshHalfExtents.x);
            const float baseScaleY = safeScale(platforms[i].halfExtents.y, meshHalfExtents.y);
            const float baseScaleZ = safeScale(platforms[i].halfExtents.z, meshHalfExtents.z);
            const float scaleX = baseScaleX * PLATFORM_VISUAL_SCALE;
            const float scaleY = baseScaleY * PLATFORM_VISUAL_SCALE;
            const float scaleZ = baseScaleZ * PLATFORM_VISUAL_SCALE;

            Rasterizer::Transform visual{
                platforms[i].center.x - meshCenter.x * scaleX,
                platforms[i].center.y - meshCenter.y * scaleY,
                platforms[i].center.z - meshCenter.z * scaleZ,
                0.0f, 0.0f, 0.0f,
                scaleX,
                scaleY,
                scaleZ
            };

            auto inst = gfx.createInstance(static_cast<uint8_t>(platformVertexId),
                                           static_cast<uint8_t>(platformTriangleId),
                                           visual);
            if (inst.getStatus() == Rasterizer::StatusCode::OK) {
                platforms[i].instanceId = inst.getInstanceId();
            } else {
                platforms[i].instanceId = 0xFF;
                std::printf("Failed to create platform %zu visual instance (status=%u)\n",
                            i,
                            static_cast<unsigned>(inst.getStatus()));
            }
        } else {
            Rasterizer::Transform cubeTransform{
                platforms[i].center.x,
                platforms[i].center.y,
                platforms[i].center.z,
                0.0f, 0.0f, 0.0f,
                platforms[i].halfExtents.x * 2.0f,
                platforms[i].halfExtents.y * 2.0f,
                platforms[i].halfExtents.z * 2.0f
            };

            auto inst = gfx.createInstance(static_cast<uint8_t>(cubeVertexId),
                                           static_cast<uint8_t>(cubeTriangleId),
                                           cubeTransform);
            if (inst.getStatus() == Rasterizer::StatusCode::OK) {
                platforms[i].instanceId = inst.getInstanceId();
            } else {
                platforms[i].instanceId = 0xFF;
                std::printf("Failed to create platform %zu fallback instance (status=%u)\n",
                            i,
                            static_cast<unsigned>(inst.getStatus()));
            }
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