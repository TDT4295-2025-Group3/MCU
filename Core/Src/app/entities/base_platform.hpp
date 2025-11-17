#pragma once

#include "math.hpp"
#include "entities/entity.hpp"
#include "anim/anim.hpp"
#include "rigidbody.hpp"
#include "game_state.hpp"
#include "entities/fire.hpp"
#include "entities/fence.hpp"

namespace mcu_game
{
    class BasePlatform : public Entity
    {
    public:
        BasePlatform(Vec3 center) : transform{center.x, center.y, center.z, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f}
        {
        }
        bool init(Rasterizer::IRasterizer &gfx, GameState &gameState) override;
        void update(IInput &input, float deltaTime, GameState &gameState) override;
        void render(Rasterizer::IRasterizer &gfx) override;

    private:
        Rasterizer::Transform transform;
        Fire fire{{transform.position.x - 0.75f, transform.position.y + 0.24f, transform.position.z - 4.5f}, 160.0f};
        Fence fence{{transform.position.x + 7.0f, transform.position.y + 0.0f, transform.position.z - 1.0f}, 3.14f};
        Fence fence2{{transform.position.x - 4.0f, transform.position.y + 0.0f, transform.position.z - 7.0f}, 1.57f};
        uint32_t vertexId = 0xFF;
        uint32_t triangleId = 0xFF;
        uint32_t instanceId = 0xFF;
    };

} // namespace mcu_game
