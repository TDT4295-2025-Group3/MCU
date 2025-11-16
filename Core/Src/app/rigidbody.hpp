#pragma once

#include <vector>
#include "math.hpp"
#include "collider.hpp"
#include "platform/irasterizer.hpp" // whatever defines Rasterizer::Transform

namespace mcu_game
{
    // Rigidbody: transform + velocity + immutable box collider + collision resolution.
    //
    // Assumptions:
    //  - transform.position is the "bottom" of the object (feet for a character).
    //  - collider.center is a LOCAL offset from transform.position to the collider center.
    //  - collider.halfExtents is in local units.
    //
    // World collider center = transform.position + collider.center.

    class Rigidbody
    {
    public:
        // You MUST provide a local-space collider description.
        // collider.center: offset from bottom to collider center
        // collider.halfExtents: half sizes in local space
        Rigidbody(const BoxCollider localCollider);

        // --- Accessors ---

        // Direct access to underlying transform (for rendering / yaw/pitch/roll).
        Rasterizer::Transform &getTransform() { return transform; }
        const Rasterizer::Transform &getTransform() const { return transform; }

        // Collider is immutable from the outside.
        const BoxCollider &getCollider() const { return collider; }

        // "Bottom" position is simply transform.position.
        Vec3 getBottomPosition() const { return transform.position; }
        void setBottomPosition(const Vec3 &bottomPos) { transform.position = bottomPos; }

        // World-space collider center.
        Vec3 getWorldColliderCenter() const { return transform.position + collider.center; }

        // Velocity.
        const Vec3 getVelocity() const { return velocity; }
        void setVelocity(const Vec3 &v) { velocity = v; }
        void addVelocity(const Vec3 &dv) { velocity += dv; }

        bool isGrounded() const { return grounded; }

        // --- Main physics step ---

        // One-call physics step: integrate + swept & penetration collisions.
        void update(float dt, const std::vector<BoxCollider> &worldColliders);

    private:
        Rasterizer::Transform transform{};
        Vec3 velocity{0.0f, 0.0f, 0.0f};
        BoxCollider collider; // immutable local collider
        bool grounded{false};

        Vec3 previousBottom{0.0f, 0.0f, 0.0f};

        // Internal step stages (not exposed).
        void beginStep();
        void integrate(float dt);
        void resolveCollisions(const std::vector<BoxCollider> &worldColliders);

        // Swept AABB vs AABB (treat this rigidbody as a moving box, colliding with 'box').
        // startCenter, deltaCenter are world-space collider centers.
        bool sweepAgainstBox(const BoxCollider &box,
                             const Vec3 &startCenter,
                             const Vec3 &deltaCenter,
                             float &outTime,
                             Vec3 &outNormal) const;

        // Resolve static penetration for one collider.
        // workingCenter is this rigidbody's WORLD collider center.
        bool resolvePenetrationOne(const BoxCollider &box,
                                   Vec3 &workingCenter,
                                   Vec3 &workingVelocity,
                                   bool &groundedFlag) const;
    };

} // namespace mcu_game
