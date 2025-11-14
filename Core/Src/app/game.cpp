#include "game.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <limits>

#include "constants.hpp"
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

// float yaw_value = 0.0f;

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

        static const Rasterizer::Vertex playerVerts[] = {
            {-0.372206f, 0.922804f, 0.117351f, 4, 6, 7},
            {-0.294266f, 1.315341f, 0.153131f, 8, 6, 4},
            {-0.377918f, 0.736952f, -0.3001f, 3, 5, 7},
            {-0.193431f, 1.265498f, -0.379793f, 7, 6, 4},
            {-0.298027f, 1.621653f, -0.241161f, 8, 6, 4},
            {-0.0f, 0.75666f, -0.3707f, 3, 5, 7},
            {-0.0f, 0.791562f, 0.227133f, 3, 5, 7},
            {0.078933f, 1.116246f, 0.284155f, 6, 6, 5},
            {0.288894f, 1.647328f, 0.095893f, 8, 7, 4},
            {-0.406813f, 0.692334f, 0.018486f, 3, 5, 7},
            {-0.13255f, 0.645191f, -0.254055f, 3, 5, 7},
            {-0.190031f, 0.391519f, 0.059896f, 4, 5, 5},
            {-0.572271f, 0.855103f, -0.126369f, 15, 12, 9},
            {-0.197689f, 0.440266f, -0.212413f, 4, 5, 6},
            {-0.139117f, 0.172127f, 0.082632f, 4, 3, 2},
            {-0.401128f, 0.862653f, -0.15223f, 15, 12, 9},
            {-0.493191f, 1.296834f, 0.043852f, 11, 9, 6},
            {-0.413326f, -0.017089f, -0.292744f, 4, 3, 2},
            {-0.393828f, 0.060416f, 0.361951f, 4, 3, 2},
            {-0.119969f, 0.056094f, 0.351868f, 4, 3, 2},
            {-0.418452f, -0.026773f, 0.320846f, 4, 3, 2},
            {-0.122091f, -0.02244f, 0.32219f, 4, 3, 2},
            {-0.121572f, -0.01996f, -0.297203f, 4, 3, 2},
            {-0.603075f, 0.531982f, 0.114052f, 15, 12, 9},
            {-0.629852f, 0.955237f, 0.033903f, 15, 12, 9},
            {-0.513301f, 0.516822f, 0.165293f, 15, 12, 9},
            {-0.495093f, 0.496206f, -0.214636f, 15, 12, 9},
            {-0.590113f, 0.399223f, 0.114102f, 15, 12, 9},
            {-0.520376f, 0.393617f, 0.044668f, 15, 12, 9},
            {-0.555402f, 0.379314f, -0.160396f, 15, 12, 9},
            {-0.232774f, 1.889844f, 0.172916f, 15, 12, 9},
            {-0.232774f, 2.355396f, 0.172916f, 4, 2, 1},
            {-0.232774f, 1.889844f, -0.292633f, 4, 2, 1},
            {-0.232774f, 2.355396f, -0.292633f, 5, 3, 2},
            {0.232774f, 1.889844f, 0.172916f, 15, 12, 9},
            {0.232774f, 2.355396f, 0.172916f, 4, 3, 2},
            {0.232774f, 1.889844f, -0.292633f, 4, 2, 1},
            {0.232774f, 2.355396f, -0.292633f, 4, 2, 1},
            {0.371711f, 0.924691f, 0.118604f, 4, 6, 7},
            {0.294588f, 1.314966f, 0.152443f, 8, 6, 4},
            {0.373243f, 0.738218f, -0.304835f, 3, 5, 7},
            {0.196992f, 1.270526f, -0.374728f, 8, 6, 4},
            {0.274409f, 1.651376f, -0.242778f, 8, 7, 4},
            {0.406819f, 0.692331f, 0.018483f, 3, 5, 7},
            {0.10564f, 0.683585f, -0.290428f, 3, 5, 7},
            {0.128475f, 0.677521f, 0.057806f, 3, 5, 7},
            {0.177361f, 0.555788f, 0.050681f, 6, 6, 7},
            {0.575765f, 0.862913f, -0.135176f, 15, 12, 9},
            {0.412178f, 0.172589f, 0.072456f, 4, 3, 2},
            {0.178395f, 0.329015f, -0.227698f, 4, 4, 4},
            {0.410539f, 0.855192f, -0.16511f, 15, 12, 9},
            {0.502294f, 1.28352f, 0.040683f, 9, 7, 5},
            {0.412473f, -0.019705f, -0.293755f, 4, 3, 2},
            {0.414993f, 0.05389f, 0.349911f, 4, 3, 2},
            {0.416604f, -0.026948f, 0.321731f, 4, 3, 2},
            {0.122853f, -0.01808f, -0.297085f, 4, 3, 2},
            {0.581161f, 0.867211f, 0.029435f, 15, 12, 9},
            {0.507092f, 0.518863f, 0.145131f, 15, 12, 9},
            {0.499477f, 0.512177f, -0.235661f, 15, 12, 9},
            {0.605029f, 0.434461f, -0.162276f, 15, 12, 9},
            {0.559576f, 0.392028f, -0.184524f, 15, 12, 9},
            {-0.30008f, 1.036123f, -0.351689f, 5, 3, 2},
            {-0.277347f, 1.590498f, -0.193395f, 5, 3, 2},
            {-0.30008f, 1.191512f, -0.827059f, 5, 3, 2},
            {-0.277347f, 1.734116f, -0.632753f, 8, 5, 2},
            {0.30008f, 1.036123f, -0.351689f, 5, 3, 2},
            {0.277347f, 1.590498f, -0.193395f, 6, 4, 2},
            {0.30008f, 1.191512f, -0.827059f, 5, 3, 2},
            {0.277347f, 1.734116f, -0.632753f, 8, 5, 2},
            {-0.487258f, 1.298812f, -0.155154f, 9, 7, 5},
            {0.339096f, 1.30085f, 0.092609f, 8, 7, 4},
            {0.495092f, 1.287585f, -0.157816f, 9, 7, 5},
            {-0.445594f, 0.718248f, -0.114959f, 4, 5, 7},
            {-0.283136f, 1.239534f, -0.116171f, 8, 6, 4},
            {-0.203976f, 0.812008f, -0.417419f, 3, 5, 7},
            {-0.317807f, 1.683937f, -0.066519f, 8, 6, 4},
            {-0.307552f, 1.626423f, 0.094577f, 8, 7, 4},
            {-0.225153f, 1.513046f, 0.195852f, 8, 6, 4},
            {-0.221964f, 1.50525f, -0.373789f, 8, 6, 4},
            {-0.096692f, 1.322622f, -0.462605f, 8, 6, 4},
            {-0.194256f, 0.905436f, 0.253545f, 4, 6, 7},
            {0.01941f, 0.710115f, -0.099875f, 3, 5, 7},
            {-0.12558f, 1.076105f, -0.461855f, 6, 6, 6},
            {-0.28007f, 0.640747f, -0.30629f, 3, 5, 7},
            {-0.267189f, 0.675074f, 0.081321f, 3, 5, 7},
            {-0.049214f, 0.685841f, -0.105737f, 3, 5, 7},
            {-0.33318f, 1.306456f, 0.098625f, 8, 7, 4},
            {-0.296191f, 0.419135f, -0.26207f, 4, 5, 6},
            {-0.294086f, 0.413577f, 0.107495f, 4, 5, 6},
            {-0.142448f, 0.535185f, -0.084621f, 4, 5, 7},
            {-0.486253f, 0.879685f, -0.179363f, 15, 12, 9},
            {-0.40898f, 0.494599f, 0.05835f, 4, 6, 7},
            {-0.143776f, 0.655018f, 0.041106f, 3, 5, 7},
            {-0.274199f, 0.192989f, 0.130322f, 5, 4, 2},
            {-0.374653f, 0.91458f, -0.039066f, 15, 12, 9},
            {-0.530229f, 0.905207f, 0.095933f, 15, 12, 9},
            {-0.407573f, 0.53277f, -0.216476f, 4, 5, 7},
            {-0.289839f, 1.262227f, -0.06981f, 9, 7, 4},
            {-0.519274f, 1.319795f, -0.056376f, 10, 8, 6},
            {-0.413651f, 1.260752f, 0.114264f, 10, 8, 6},
            {-0.40172f, 1.234743f, -0.226316f, 9, 8, 5},
            {-0.300176f, 1.261644f, -0.22921f, 9, 7, 5},
            {-0.267094f, -0.006211f, -0.351716f, 4, 3, 2},
            {-0.113726f, 0.205743f, -0.116112f, 4, 3, 2},
            {-0.446705f, 0.152647f, -0.122554f, 5, 3, 2},
            {-0.271191f, 0.06622f, 0.394971f, 4, 3, 2},
            {-0.271886f, -0.016269f, 0.372081f, 4, 3, 2},
            {-0.060616f, -0.011416f, 0.204069f, 4, 3, 2},
            {-0.482632f, 0.002865f, 0.222942f, 5, 3, 2},
            {-0.414387f, 0.165893f, 0.07334f, 6, 4, 3},
            {-0.057623f, -0.007988f, -0.129499f, 4, 3, 2},
            {-0.484059f, -0.006847f, -0.127805f, 5, 3, 2},
            {-0.432102f, 0.857954f, 0.079402f, 15, 12, 9},
            {-0.552635f, 0.543409f, 0.192602f, 15, 12, 9},
            {-0.636072f, 0.963132f, -0.047335f, 15, 12, 9},
            {-0.62174f, 0.415301f, -0.052045f, 15, 12, 9},
            {-0.544849f, 0.513576f, -0.250835f, 15, 12, 9},
            {-0.283783f, 1.838835f, -0.059859f, 9, 7, 5},
            {-0.283783f, 2.12262f, 0.223925f, 9, 7, 5},
            {-0.283783f, 2.406406f, -0.059859f, 4, 3, 2},
            {-0.283783f, 2.12262f, -0.343642f, 4, 3, 2},
            {0.0f, 1.838835f, -0.343642f, 4, 2, 1},
            {0.0f, 2.406406f, -0.343642f, 4, 3, 2},
            {0.283783f, 2.12262f, -0.343642f, 4, 2, 1},
            {0.283783f, 1.838835f, -0.059859f, 9, 7, 5},
            {0.283783f, 2.406406f, -0.059859f, 4, 2, 1},
            {0.283783f, 2.12262f, 0.223925f, 10, 7, 5},
            {0.0f, 1.838835f, 0.223925f, 15, 12, 9},
            {0.0f, 2.406406f, 0.223925f, 4, 2, 1},
            {0.283651f, 1.241585f, -0.125348f, 8, 6, 4},
            {0.224142f, 0.745613f, -0.390319f, 3, 5, 7},
            {0.298759f, 1.701683f, -0.065441f, 8, 7, 4},
            {0.214721f, 1.524235f, 0.196857f, 8, 6, 4},
            {0.222861f, 1.500818f, -0.369528f, 8, 7, 4},
            {0.098813f, 1.176497f, -0.475059f, 7, 6, 5},
            {0.198915f, 0.906006f, 0.2527f, 4, 6, 7},
            {0.274035f, 0.671561f, 0.082518f, 4, 6, 7},
            {0.44467f, 0.710822f, -0.119267f, 4, 6, 7},
            {0.288066f, 0.592376f, -0.279152f, 3, 5, 7},
            {0.295297f, 0.419816f, 0.107261f, 6, 6, 6},
            {0.152489f, 0.506182f, -0.082377f, 5, 6, 7},
            {0.491848f, 0.885968f, -0.189935f, 15, 12, 9},
            {0.17928f, 0.551628f, -0.217915f, 3, 5, 7},
            {0.407044f, 0.494662f, 0.05994f, 6, 6, 7},
            {0.322386f, 1.175901f, -0.205045f, 9, 7, 5},
            {0.444533f, 0.170245f, -0.118468f, 4, 3, 2},
            {0.285559f, 0.298816f, -0.281142f, 4, 4, 3},
            {0.275282f, 0.169167f, 0.143211f, 4, 3, 2},
            {0.369427f, 0.921706f, -0.049546f, 15, 11, 9},
            {0.612571f, 0.936354f, -0.054695f, 15, 12, 9},
            {0.500399f, 0.892275f, 0.087155f, 15, 12, 9},
            {0.154477f, 0.203891f, 0.059456f, 5, 3, 2},
            {0.408138f, 0.559924f, -0.215951f, 4, 5, 7},
            {0.290183f, 1.263202f, -0.073178f, 8, 7, 4},
            {0.524215f, 1.315037f, -0.059043f, 9, 7, 5},
            {0.417222f, 1.253808f, 0.110185f, 8, 7, 4},
            {0.403399f, 1.237172f, -0.229396f, 9, 7, 5},
            {0.277078f, 1.296158f, -0.274831f, 8, 6, 4},
            {0.476531f, -0.007803f, -0.151227f, 4, 3, 2},
            {0.268f, -0.006517f, -0.351426f, 4, 3, 2},
            {0.167414f, -0.024838f, 0.358526f, 4, 3, 2},
            {0.097602f, 0.163521f, -0.108111f, 4, 3, 2},
            {0.267684f, 0.053915f, 0.404059f, 4, 3, 2},
            {0.132268f, 0.050215f, 0.363697f, 4, 3, 2},
            {0.059281f, 0.006613f, 0.229208f, 4, 3, 2},
            {0.485828f, -0.009071f, 0.192132f, 4, 3, 2},
            {0.056954f, -0.008039f, -0.132036f, 4, 3, 2},
            {0.541438f, 0.547015f, -0.27244f, 15, 12, 9},
            {0.420206f, 0.857557f, 0.064144f, 15, 12, 9},
            {0.550465f, 0.554942f, 0.178958f, 15, 12, 9},
            {0.482057f, 0.508096f, -0.04471f, 15, 12, 9},
            {0.565184f, 0.396953f, 0.095882f, 15, 12, 9},
            {0.62449f, 0.428899f, -0.046168f, 15, 12, 9},
            {0.567257f, 0.36289f, -0.043864f, 15, 12, 9},
            {0.609682f, 0.438547f, 0.070359f, 15, 12, 9},
            {-0.370504f, 1.053721f, -0.608691f, 5, 3, 2},
            {-0.351981f, 1.296929f, -0.222429f, 5, 3, 2},
            {-0.333458f, 1.722404f, -0.393757f, 7, 4, 2},
            {-0.351981f, 1.479195f, -0.780019f, 7, 4, 2},
            {-0.0f, 1.149649f, -0.902157f, 5, 3, 2},
            {0.0f, 1.808741f, -0.65788f, 8, 5, 2},
            {0.351981f, 1.479195f, -0.780019f, 7, 4, 2},
            {0.370504f, 1.053721f, -0.608691f, 5, 3, 2},
            {0.333458f, 1.722404f, -0.393757f, 7, 5, 2},
            {0.351981f, 1.29693f, -0.222429f, 6, 3, 2},
            {-0.0f, 0.957792f, -0.315224f, 5, 3, 2},
            {0.0f, 1.636067f, -0.129634f, 6, 3, 2},
            {-0.57925f, 0.358978f, -0.022797f, 15, 12, 9},
            {-0.39083f, 2.12262f, -0.059859f, 7, 5, 3},
            {0.0f, 2.12262f, -0.45069f, 4, 2, 1},
            {0.39083f, 2.12262f, -0.059859f, 7, 5, 3},
            {0.0f, 2.12262f, 0.330972f, 9, 7, 5},
            {0.0f, 1.731786f, -0.059859f, 9, 7, 5},
            {0.0f, 2.513455f, -0.059859f, 4, 2, 2},
            {-0.484753f, 1.388062f, -0.501224f, 6, 4, 2},
            {-0.0f, 1.513572f, -0.885184f, 7, 4, 2},
            {0.484753f, 1.388062f, -0.501224f, 6, 4, 2},
            {0.0f, 1.262553f, -0.117264f, 5, 3, 2},
            {-0.0f, 0.927602f, -0.649229f, 5, 3, 2},
            {0.0f, 1.848523f, -0.353219f, 7, 4, 2},
        };

        constexpr std::size_t playerVertexCount = sizeof(playerVerts) / sizeof(playerVerts[0]);
        const auto createPlayerVert = gfx.createVertex(playerVerts, static_cast<uint16_t>(playerVertexCount));
        if (!createPlayerVert.isSuccess()) {
            std::printf("Failed to create player vertex buffer (status=%u)\n", static_cast<unsigned>(createPlayerVert.getStatus()));
            return;
        }
        playerVertexId = createPlayerVert.getVertexId();

        static const Rasterizer::Triangle playerTris[] = {
            {0, 73, 72},
            {7, 80, 6},
            {3, 78, 79},
            {78, 4, 42},
            {4, 78, 69},
            {101, 100, 78},
            {72, 9, 0},
            {3, 79, 82},
            {74, 82, 5},
            {80, 92, 6},
            {9, 84, 80},
            {2, 83, 96},
            {84, 91, 88},
            {90, 12, 69},
            {9, 72, 91},
            {92, 89, 85},
            {89, 13, 10},
            {13, 103, 22},
            {89, 11, 103},
            {87, 17, 96},
            {76, 98, 16},
            {77, 76, 99},
            {14, 19, 103},
            {19, 21, 107},
            {17, 111, 104},
            {108, 20, 18},
            {93, 109, 105},
            {105, 19, 14},
            {12, 115, 114},
            {95, 24, 113},
            {113, 25, 112},
            {90, 15, 116},
            {26, 29, 116},
            {116, 29, 115},
            {112, 28, 94},
            {15, 28, 26},
            {187, 27, 115},
            {187, 115, 29},
            {28, 29, 26},
            {23, 27, 113},
            {115, 27, 23},
            {38, 129, 39},
            {135, 7, 6},
            {78, 42, 133},
            {133, 41, 134},
            {71, 156, 133},
            {133, 157, 41},
            {132, 7, 39},
            {43, 137, 38},
            {137, 152, 40},
            {81, 45, 6},
            {130, 44, 5},
            {138, 142, 44},
            {148, 168, 70},
            {46, 136, 45},
            {47, 141, 71},
            {49, 140, 142},
            {49, 161, 140},
            {161, 151, 140},
            {140, 151, 46},
            {52, 146, 152},
            {151, 147, 139},
            {150, 56, 51},
            {8, 154, 131},
            {155, 51, 8},
            {144, 153, 157},
            {52, 158, 165},
            {164, 161, 166},
            {158, 52, 145},
            {164, 160, 163},
            {48, 53, 145},
            {53, 54, 165},
            {163, 147, 151},
            {147, 53, 48},
            {54, 55, 52},
            {54, 160, 55},
            {163, 160, 162},
            {160, 54, 162},
            {56, 172, 149},
            {172, 47, 149},
            {57, 169, 168},
            {167, 58, 50},
            {59, 167, 47},
            {59, 60, 167},
            {60, 58, 167},
            {50, 170, 148},
            {170, 168, 148},
            {57, 171, 169},
            {171, 174, 169},
            {169, 174, 56},
            {16, 98, 24},
            {70, 150, 155},
            {0, 1, 73},
            {0, 80, 77},
            {0, 77, 1},
            {7, 77, 80},
            {75, 131, 42},
            {75, 42, 4},
            {8, 131, 75},
            {8, 75, 76},
            {76, 77, 132},
            {76, 132, 8},
            {78, 3, 101},
            {69, 78, 100},
            {7, 132, 77},
            {74, 2, 3},
            {74, 3, 82},
            {85, 10, 5},
            {85, 5, 81},
            {0, 9, 80},
            {92, 80, 84},
            {92, 85, 81},
            {92, 81, 6},
            {10, 83, 74},
            {10, 74, 5},
            {2, 74, 83},
            {13, 87, 83},
            {13, 83, 10},
            {11, 92, 84},
            {11, 84, 88},
            {96, 83, 87},
            {9, 91, 84},
            {12, 114, 98},
            {12, 98, 69},
            {3, 2, 72},
            {3, 72, 73},
            {72, 2, 96},
            {85, 89, 10},
            {92, 11, 89},
            {22, 103, 110},
            {98, 114, 24},
            {109, 93, 88},
            {109, 88, 91},
            {89, 103, 13},
            {11, 14, 103},
            {93, 14, 11},
            {93, 11, 88},
            {104, 72, 96},
            {104, 96, 17},
            {104, 109, 91},
            {104, 91, 72},
            {69, 98, 75},
            {69, 75, 4},
            {76, 75, 98},
            {86, 1, 77},
            {86, 77, 99},
            {101, 3, 73},
            {101, 73, 97},
            {99, 76, 16},
            {97, 73, 1},
            {97, 1, 86},
            {102, 87, 13},
            {102, 13, 22},
            {87, 102, 17},
            {107, 110, 103},
            {107, 103, 19},
            {106, 105, 18},
            {106, 18, 20},
            {106, 21, 19},
            {106, 19, 105},
            {22, 110, 107},
            {22, 107, 21},
            {17, 20, 108},
            {17, 108, 111},
            {109, 104, 108},
            {104, 111, 108},
            {109, 108, 18},
            {93, 105, 14},
            {105, 109, 18},
            {17, 22, 20},
            {20, 22, 21},
            {106, 20, 21},
            {115, 23, 24},
            {115, 24, 114},
            {102, 22, 17},
            {27, 28, 25},
            {27, 25, 113},
            {95, 113, 112},
            {113, 24, 23},
            {90, 116, 12},
            {116, 15, 26},
            {15, 94, 28},
            {112, 25, 28},
            {29, 28, 187},
            {28, 27, 187},
            {188, 117, 30},
            {188, 30, 118},
            {12, 116, 115},
            {188, 118, 31},
            {188, 31, 119},
            {33, 120, 188},
            {33, 188, 119},
            {32, 117, 188},
            {32, 188, 120},
            {189, 121, 32},
            {189, 32, 120},
            {189, 120, 33},
            {189, 33, 122},
            {37, 123, 189},
            {37, 189, 122},
            {36, 121, 189},
            {36, 189, 123},
            {190, 124, 36},
            {190, 36, 123},
            {190, 123, 37},
            {190, 37, 125},
            {35, 126, 190},
            {35, 190, 125},
            {34, 124, 190},
            {34, 190, 126},
            {191, 127, 34},
            {191, 34, 126},
            {191, 126, 35},
            {191, 35, 128},
            {31, 118, 191},
            {31, 191, 128},
            {30, 127, 191},
            {30, 191, 118},
            {192, 117, 32},
            {192, 32, 121},
            {192, 121, 36},
            {192, 36, 124},
            {34, 127, 192},
            {34, 192, 124},
            {30, 117, 192},
            {30, 192, 127},
            {193, 125, 37},
            {193, 37, 122},
            {193, 122, 33},
            {193, 33, 119},
            {31, 128, 193},
            {31, 193, 119},
            {35, 125, 193},
            {35, 193, 128},
            {41, 129, 137},
            {41, 137, 40},
            {38, 137, 129},
            {7, 135, 38},
            {7, 38, 39},
            {133, 134, 79},
            {133, 79, 78},
            {157, 133, 156},
            {157, 156, 144},
            {42, 71, 133},
            {41, 40, 130},
            {41, 130, 134},
            {82, 134, 130},
            {82, 130, 5},
            {135, 6, 45},
            {135, 45, 136},
            {134, 82, 79},
            {38, 135, 136},
            {38, 136, 43},
            {138, 130, 40},
            {138, 40, 152},
            {5, 44, 81},
            {81, 44, 142},
            {81, 142, 140},
            {130, 138, 44},
            {46, 139, 136},
            {136, 139, 143},
            {136, 143, 43},
            {43, 143, 137},
            {45, 81, 140},
            {45, 140, 46},
            {166, 161, 49},
            {166, 49, 55},
            {154, 51, 56},
            {154, 56, 149},
            {47, 71, 154},
            {47, 154, 149},
            {142, 138, 146},
            {142, 146, 49},
            {152, 146, 138},
            {48, 143, 139},
            {48, 139, 147},
            {46, 151, 139},
            {145, 137, 143},
            {145, 143, 48},
            {152, 137, 145},
            {152, 145, 52},
            {71, 42, 131},
            {71, 131, 154},
            {70, 168, 150},
            {8, 51, 154},
            {132, 39, 70},
            {132, 70, 155},
            {129, 41, 157},
            {129, 157, 153},
            {132, 155, 8},
            {39, 129, 153},
            {39, 153, 70},
            {49, 146, 159},
            {49, 159, 55},
            {52, 159, 146},
            {151, 161, 164},
            {151, 164, 163},
            {164, 166, 55},
            {164, 55, 160},
            {165, 158, 145},
            {162, 54, 53},
            {173, 60, 59},
            {173, 59, 172},
            {145, 53, 165},
            {163, 162, 147},
            {147, 162, 53},
            {165, 54, 52},
            {159, 52, 55},
            {168, 169, 150},
            {150, 169, 56},
            {47, 167, 141},
            {141, 167, 50},
            {50, 58, 170},
            {170, 57, 168},
            {174, 171, 173},
            {174, 173, 172},
            {170, 58, 60},
            {170, 60, 173},
            {171, 57, 170},
            {171, 170, 173},
            {194, 175, 61},
            {194, 61, 176},
            {56, 174, 172},
            {172, 59, 47},
            {194, 176, 62},
            {194, 62, 177},
            {64, 178, 194},
            {64, 194, 177},
            {63, 175, 194},
            {63, 194, 178},
            {195, 179, 63},
            {195, 63, 178},
            {195, 178, 64},
            {195, 64, 180},
            {68, 181, 195},
            {68, 195, 180},
            {67, 179, 195},
            {67, 195, 181},
            {196, 182, 67},
            {196, 67, 181},
            {196, 181, 68},
            {196, 68, 183},
            {66, 184, 196},
            {66, 196, 183},
            {65, 182, 196},
            {65, 196, 184},
            {197, 185, 65},
            {197, 65, 184},
            {197, 184, 66},
            {197, 66, 186},
            {62, 176, 197},
            {62, 197, 186},
            {61, 185, 197},
            {61, 197, 176},
            {198, 175, 63},
            {198, 63, 179},
            {198, 179, 67},
            {198, 67, 182},
            {65, 185, 198},
            {65, 198, 182},
            {61, 175, 198},
            {61, 198, 185},
            {199, 183, 68},
            {199, 68, 180},
            {199, 180, 64},
            {199, 64, 177},
            {62, 186, 199},
            {62, 199, 177},
            {66, 183, 199},
            {66, 199, 186},
            {24, 95, 99},
            {24, 99, 16},
            {95, 112, 86},
            {95, 86, 99},
            {90, 100, 101},
            {90, 101, 15},
            {112, 94, 97},
            {112, 97, 86},
            {100, 90, 69},
            {15, 101, 97},
            {15, 97, 94},
            {144, 156, 141},
            {144, 141, 50},
            {155, 150, 51},
            {71, 141, 156},
            {153, 144, 50},
            {153, 50, 148},
            {153, 148, 70},
        };

        constexpr std::size_t playerTriangleCount = sizeof(playerTris) / sizeof(playerTris[0]);
        const auto createPlayerTri = gfx.createTriangle(playerTris, static_cast<uint16_t>(playerTriangleCount));
        if (!createPlayerTri.isSuccess()) {
            std::printf("Failed to create player triangle buffer (status=%u)\n", static_cast<unsigned>(createPlayerTri.getStatus()));
            return;
        }
        playerTriangleId = createPlayerTri.getTriangleId();
    }

    if (playerVertexId == 0xFF || playerTriangleId == 0xFF) {
        std::printf("Player geometry unavailable, aborting init\n");
        return;
    }

    Rasterizer::Transform playerTransform {
        0.0f, PLAYER_HALF_EXTENTS.y + PLAYER_VISUAL_Y_OFFSET, 0.0f,
        0.0f, 0.0f, 0.0f,
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

    initialize_platforms(cubeVertexId, cubeTriangleId);

    const Rasterizer::Transform initialCameraTransform{
        camera.getPosition().x,
        camera.getPosition().y,
        camera.getPosition().z,
        camera.getPitch(),
        camera.getYaw(),
        0.0f,
        1.0f, 1.0f, 1.0f
    };
    const auto cameraResp = gfx.updateCamera(initialCameraTransform);
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
            0.0f, 0.0f, 0.0f,
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
    gfx.updateCamera(camT);

    // yaw_value += 2 * 3.14159265f / (10*60.0f);

    gfx.end_frame();
}

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
    }
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