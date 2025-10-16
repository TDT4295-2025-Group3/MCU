#pragma once
#include "iinput.hpp"
#include "irasterizer.hpp"
#include "itimer.hpp"
#include "player.hpp"
#include "camera.hpp"

#include <array>
#include <cstddef>

class Game {
public:
    Game(Rasterizer::IRasterizer& gfx, IInput& in, ITimer& time)
        : gfx(gfx), input(in), timer(time) {}

    void init();
    void tick_once();
private:
    void tick_logic();
    void tick_graphics();
    void handle_player_collisions(const mcu_game::Vec3& previousPosition);
    bool sweep_against_box(const mcu_game::Vec3& boxCenter,
                           const mcu_game::Vec3& boxHalfExtents,
                           const mcu_game::Vec3& start,
                           const mcu_game::Vec3& delta,
                           float& outTime,
                           mcu_game::Vec3& outNormal) const;
    void initialize_platforms(uint32_t cubeVertexId, uint32_t cubeTriangleId);
    bool initialized = false;
    mcu_game::Player player{};
    mcu_game::Camera camera{};

    uint32_t instanceCubeId = 0xFF;
    uint32_t instancePyrId  = 0xFF;
    uint32_t instancePlaneId = 0xFF;
    uint32_t cubeVertexId = 0xFF;
    uint32_t cubeTriangleId = 0xFF;

    struct Platform {
        mcu_game::Vec3 center{0.0f, 0.0f, 0.0f};
        mcu_game::Vec3 halfExtents{0.5f, 0.5f, 0.5f};
        uint32_t instanceId = 0xFF;
    };

    static constexpr std::size_t PLATFORM_COUNT = 3;
    std::array<Platform, PLATFORM_COUNT> platforms{};
    mcu_game::Vec3 groundCenter{0.0f, -0.05f, 0.0f};
    mcu_game::Vec3 groundHalfExtents{8.0f, 0.05f, 8.0f};
private:
    Rasterizer::IRasterizer& gfx;
    IInput&      input;
    ITimer&      timer;
    uint32_t next_tick_ms;
    uint32_t next_frame_ms;
};
