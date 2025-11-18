#pragma once

#include "math.hpp"

#include "entities/entity.hpp"
#include "anim/anim.hpp"
#include "rigidbody.hpp"
#include "game_state.hpp"

namespace mcu_game
{
    class Model : public Entity
    {
    public:
        Model(Vec3 position, Vec3 rotation, Vec3 scale, mcu_game::assets::baked::MeshId meshId)
            : transform{position.x, position.y, position.z,
                        rotation.x, rotation.y, rotation.z,
                        scale.x, scale.y, scale.z},
              meshId(meshId)
        {
        }
        bool init(GameState &gameState) override;

    private:
        mcu_game::assets::baked::MeshId meshId;

        Rasterizer::Transform transform;
        uint32_t vertexId = 0xFF;
        uint32_t triangleId = 0xFF;
        uint32_t instanceId = 0xFF;
    };

} // namespace mcu_game
