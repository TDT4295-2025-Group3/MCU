#pragma once

#include "math.hpp"

#include "entities/entity.hpp"
#include "anim/anim.hpp"
#include "rigidbody.hpp"
#include "game_state.hpp"

namespace mcu_game
{
    class Logo : public Entity
    {
    public:
        Logo(Vec3 center, float rotation) : startPosition(center), transform{center.x, center.y, center.z, 0.0f, rotation, 0.0f, 1.4f, 1.4f, 1.4f}
        {
        }
        bool init(GameState &gameState) override;
        void update(float deltaTime, GameState &gameState) override;
        void render(GameState &gameState) override;

    private:
        Vec3 startPosition;
        Rasterizer::Transform transform;
        Animator animator{};
        uint32_t instanceLogoId = 0xFF;
        uint32_t vertexLogoId = 0xFF;
        uint32_t triangleLogoId = 0xFF;
        uint32_t vertexPressXId = 0xFF;
        uint32_t trianglePressXId = 0xFF;
        uint32_t instancePressXId = 0xFF;
        uint32_t emptyVertexId = 0xFF;
        uint32_t emptyTriangleId = 0xFF;
    };

} // namespace mcu_game
