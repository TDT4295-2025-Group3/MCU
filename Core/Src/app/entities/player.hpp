#pragma once

#include "math.hpp"

#include "entities/entity.hpp"
#include "anim/anim.hpp"
#include "rigidbody.hpp"
#include "game_state.hpp"

namespace mcu_game
{
    struct PlayerConfig
    {
        float jumpHeight = 3.5f;
        float timeToApex = 0.45f;

        float gravity;
        float jumpVelocity;

        float moveSpeed = 9.0f;

        float coyoteTime = 0.12f;
        float jumpBufferTime = 0.15f;

        float airControlFactor = 0.9f;
        float friction = 10.0f;
        float turnSpeed = 15.0f;
        float fallGravityMultiplier = 1.6f;
        float lowJumpGravityMultiplier = 1.4f;

        float fall_rumble_threshold = -5.0f;

        PlayerConfig()
        {
            gravity = -2.0f * jumpHeight / (timeToApex * timeToApex);
            jumpVelocity = 2.0f * jumpHeight / timeToApex;
        }

        BoxCollider collider{
            {0.0f, 1.1f, 0.0f},
            {0.4f, 1.1f, 0.4f}};
    };

    class Player : public Entity
    {
    public:
        Player(Vec3 position = {0.0f, 0.0f, 0.0f})
        {
            startPosition = position;
            body.setBottomPosition(position);
        }
        bool init(GameState &gameState) override;
        void update(float deltaTime, GameState &gameState) override;
        void render(GameState &gameState) override;

        Vec3 getPosition() const { return body.getBottomPosition(); }
        Vec3 getVelocity() const { return body.getVelocity(); }

    private:
        PlayerConfig playerConfig{};
        Rigidbody body{playerConfig.collider};
        Vec3 startPosition{0.0f, 0.0f, 0.0f};
        Animator animator{};

        float coyoteTimer{0.0f};
        float jumpBufferTimer{0.0f};
        bool lastJumpPressed{false};

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
        uint32_t vertexSleepId = 0xFF;
        uint32_t triangleSleepId = 0xFF;
        uint32_t instanceId = 0xFF;
    };

} // namespace mcu_game
