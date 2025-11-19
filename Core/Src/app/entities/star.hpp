#pragma once

#include "math.hpp"

#include "entities/entity.hpp"
#include "anim/anim.hpp"
#include "rigidbody.hpp"
#include "game_state.hpp"

#include <cstdlib> // for std::rand, RAND_MAX

namespace mcu_game
{
    class Star : public Entity
    {
    public:
        // startPos = initial position
        Star(Vec3 startPos, float rotation)
            : transform{startPos.x, startPos.y, startPos.z,
                        0.0f, rotation, 0.0f,
                        1.0f, 1.0f, 1.0f}
        {
            // random delay in [0, 1] seconds
            float r = static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX);
            startDelay = r;
            animationStarted = false;
        }

        bool init(GameState &gameState) override;
        void update(float deltaTime, GameState &gameState) override;
        void render(GameState &gameState) override;

    private:
        Rasterizer::Transform transform;
        Animator animator{};
        uint32_t vertexId = 0xFF;
        uint32_t triangleId = 0xFF;
        uint32_t emptyVertexId = 0xFF;
        uint32_t emptyTriangleId = 0xFF;
        uint32_t instanceId = 0xFF;

        float startDelay = 0.0f; // seconds until animation starts
        bool animationStarted = false;
        bool lastIsVisible{true};
    };

} // namespace mcu_game
