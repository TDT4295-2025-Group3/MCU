#pragma once

#include "math.hpp"
#include "input.hpp"
#include "entity.hpp"
#include "anim/anim.hpp"
#include "rigidbody.hpp"
#include "game_state.hpp"

namespace mcu_game
{
    struct PlayerConfig
    {
        float moveSpeed = 4.0f;
        float airControlFactor = 1.2f;
        float jumpVelocity = 6.0f;
        float gravity = -9.8f;
        float friction = 8.0f;
        float turnSpeed = 10.0f;
        BoxCollider collider{
            {0.0f, 1.1f, 0.0f}, // center
            {0.4f, 1.1f, 0.4f}  // half extents
        };
    };

    class Player : public Entity
    {
    public:
        Player() = default;
        bool init(Rasterizer::IRasterizer &gfx, GameState &gameState) override;
        void update(const InputState &in, float deltaTime, GameState &gameState) override;
        void render(Rasterizer::IRasterizer &gfx) override;

        Vec3 getPosition() const { return body.getBottomPosition(); }
        Vec3 getVelocity() const { return body.getVelocity(); }

        const Rigidbody &getBody() const { return body; }

    private:
        PlayerConfig playerConfig{};
        Rigidbody body{playerConfig.collider};
        Animator animator{};

        uint32_t vertexIdleId = 0xFF;
        uint32_t triangleIdleId = 0xFF;
        uint32_t vertexRun1Id = 0xFF;
        uint32_t triangleRun1Id = 0xFF;
        uint32_t vertexRun2Id = 0xFF;
        uint32_t triangleRun2Id = 0xFF;
        uint32_t vertexJumpUpId = 0xFF;
        uint32_t triangleJumpUpId = 0xFF;
        uint32_t vertexJumpDownId = 0xFF;
        uint32_t triangleJumpDownId = 0xFF;
        uint32_t instanceId = 0xFF;
    };

} // namespace mcu_game
