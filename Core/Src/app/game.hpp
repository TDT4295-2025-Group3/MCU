#pragma once
#include "iinput.hpp"
#include "irasterizer.hpp"
#include "itimer.hpp"
#include "entities/player.hpp"
#include "entities/camera.hpp"

#include <array>
#include <cstddef>

class Game
{
public:
    Game(Rasterizer::IRasterizer &gfx, IInput &in, ITimer &time, const char *modelBasePath = nullptr)
        : gfx(gfx), input(in), timer(time), modelBasePath(modelBasePath) {}

    void init();
    void tick_once();
    void setModelBasePath(const char *basePath) { modelBasePath = basePath; }
    void setShowHitboxDebug(bool enable)
    {
        showHitboxDebug = enable;
        updateHitboxDebugInstance();
    }
    bool isShowingHitboxDebug() const { return showHitboxDebug; }

private:
    void tick_logic();
    void tick_graphics();
    void updateHitboxDebugInstance();
    bool loadModelGeometry(const char *relativePath,
                           uint32_t &vertexId,
                           uint32_t &triangleId,
                           bool logSuccess = true,
                           size_t *outVertexCount = nullptr,
                           size_t *outTriangleCount = nullptr);
    bool loadModelInstance(const char *relativePath, const Rasterizer::Transform &transform, uint32_t &instanceId);
    void createEntity(mcu_game::Entity *entity);
    void initialize_platforms();

    bool initialized = false;
    std::vector<mcu_game::Entity *> entities;
    mcu_game::GameState gameState{};

    uint32_t instancePyrId = 0xFF;
    uint32_t instancePlaneId = 0xFF;
    uint32_t hitboxVertexId = 0xFF; // Invisible collision prism (cube geometry)
    uint32_t hitboxTriangleId = 0xFF;
    uint32_t cubeVertexId = 0xFF;
    uint32_t cubeTriangleId = 0xFF;
    uint32_t platformVertexId = 0xFF; // Visible platform mesh
    uint32_t platformTriangleId = 0xFF;

    struct Platform
    {
        mcu_game::Vec3 center{0.0f, 0.0f, 0.0f};
        mcu_game::Vec3 halfExtents{0.5f, 0.5f, 0.5f};
        uint32_t instanceId = 0xFF;
        uint32_t hitboxInstanceId = 0xFF;
    };

    static constexpr std::size_t PLATFORM_COUNT = 3;
    std::array<Platform, PLATFORM_COUNT> platforms{};
    static constexpr std::size_t DEBUG_CUBE_COUNT = 4;
    std::array<uint32_t, DEBUG_CUBE_COUNT> debugCubeInstanceIds{};
    bool showHitboxDebug = false; // Whether to show hitbox when debugging
    uint32_t hitboxDebugInstanceId = 0xFF;
    mcu_game::Vec3 groundCenter{0.0f, -0.05f, 0.0f};
    mcu_game::Vec3 groundHalfExtents{8.0f, 0.05f, 8.0f};

private:
    Rasterizer::IRasterizer &gfx;
    IInput &input;
    ITimer &timer;
    uint32_t next_tick_ms;
    uint32_t next_frame_ms;
    const char *modelBasePath = nullptr;
};
