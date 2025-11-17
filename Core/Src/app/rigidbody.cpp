#include "rigidbody.hpp"

#include <algorithm>
#include <cmath>

namespace mcu_game
{
    namespace
    {
        constexpr float RAY_EPSILON = 1e-4f;
    }

    Rigidbody::Rigidbody(const BoxCollider localCollider)
        : collider(localCollider)
    {
    }

    // --- Internal step stages ---

    void Rigidbody::beginStep()
    {
        previousBottom = transform.position;
    }

    void Rigidbody::integrate(float dt)
    {
        transform.position += velocity * dt;
        grounded = false; // collision resolution will set this if we land
    }

    // --- Swept box vs box ---

    bool Rigidbody::sweepAgainstBox(const BoxCollider &box,
                                    const Vec3 &startCenter,
                                    const Vec3 &deltaCenter,
                                    float &outTime,
                                    Vec3 &outNormal) const
    {
        // Treat this rigidbody's collider as a moving box:
        // Expand the static box by our halfExtents and raycast a point.

        const Vec3 expandedMin = {
            box.center.x - box.halfExtents.x - collider.halfExtents.x,
            box.center.y - box.halfExtents.y - collider.halfExtents.y,
            box.center.z - box.halfExtents.z - collider.halfExtents.z};

        const Vec3 expandedMax = {
            box.center.x + box.halfExtents.x + collider.halfExtents.x,
            box.center.y + box.halfExtents.y + collider.halfExtents.y,
            box.center.z + box.halfExtents.z + collider.halfExtents.z};

        float tFirst = 0.0f;
        float tLast = 1.0f;
        Vec3 normal{0.0f, 0.0f, 0.0f};

        auto axisCheck = [&](float startCoord, float dir, float minCoord, float maxCoord, int axis) -> bool
        {
            if (std::fabs(dir) < 1e-6f)
            {
                // No movement along this axis: if we're outside the slab, no hit.
                return (startCoord >= minCoord && startCoord <= maxCoord);
            }

            float invDir = 1.0f / dir;
            float t1 = (minCoord - startCoord) * invDir;
            float t2 = (maxCoord - startCoord) * invDir;
            float entry = std::min(t1, t2);
            float exit = std::max(t1, t2);

            if (entry > tLast || exit < tFirst)
                return false;

            if (entry > tFirst)
            {
                tFirst = entry;
                normal = {0.0f, 0.0f, 0.0f};
                if (axis == 0)
                    normal.x = dir > 0.0f ? -1.0f : 1.0f;
                else if (axis == 1)
                    normal.y = dir > 0.0f ? -1.0f : 1.0f;
                else
                    normal.z = dir > 0.0f ? -1.0f : 1.0f;
            }

            tLast = std::min(tLast, exit);
            return tFirst <= tLast;
        };

        if (!axisCheck(startCenter.x, deltaCenter.x, expandedMin.x, expandedMax.x, 0))
            return false;
        if (!axisCheck(startCenter.y, deltaCenter.y, expandedMin.y, expandedMax.y, 1))
            return false;
        if (!axisCheck(startCenter.z, deltaCenter.z, expandedMin.z, expandedMax.z, 2))
            return false;

        if (tFirst < 0.0f || tFirst > 1.0f)
            return false;

        outTime = std::max(0.0f, tFirst);
        outNormal = normal;
        return true;
    }

    // --- Penetration resolution ---

    bool Rigidbody::resolvePenetrationOne(const BoxCollider &box,
                                          Vec3 &workingCenter,
                                          Vec3 &workingVelocity,
                                          bool &groundedFlag) const
    {
        const Vec3 delta = workingCenter - box.center;

        const float overlapX = (box.halfExtents.x + collider.halfExtents.x) - std::fabs(delta.x);
        const float overlapY = (box.halfExtents.y + collider.halfExtents.y) - std::fabs(delta.y);
        const float overlapZ = (box.halfExtents.z + collider.halfExtents.z) - std::fabs(delta.z);

        if (overlapX <= 0.0f || overlapY <= 0.0f || overlapZ <= 0.0f)
            return false;

        auto chooseDir = [](float d, float vel)
        {
            if (std::fabs(d) > 1e-5f)
                return d >= 0.0f ? 1.0f : -1.0f;
            if (std::fabs(vel) > 1e-5f)
                return vel >= 0.0f ? 1.0f : -1.0f;
            return 1.0f;
        };

        float minOverlap = overlapX;
        int axis = 0;
        if (overlapY < minOverlap)
        {
            minOverlap = overlapY;
            axis = 1;
        }
        if (overlapZ < minOverlap)
        {
            minOverlap = overlapZ;
            axis = 2;
        }

        const float pushDistance = minOverlap + RAY_EPSILON;

        if (axis == 0)
        {
            float dir = chooseDir(delta.x, workingVelocity.x);
            workingCenter.x += dir * pushDistance;

            if ((dir > 0.0f && workingVelocity.x < 0.0f) ||
                (dir < 0.0f && workingVelocity.x > 0.0f))
            {
                workingVelocity.x = 0.0f;
            }
        }
        else if (axis == 1)
        {
            float dir = chooseDir(delta.y, workingVelocity.y);
            workingCenter.y += dir * pushDistance;

            if (dir > 0.0f)
            {
                groundedFlag = true;
                if (workingVelocity.y < 0.0f)
                    workingVelocity.y = 0.0f;
            }
            else
            {
                if (workingVelocity.y > 0.0f)
                    workingVelocity.y = 0.0f;
            }
        }
        else
        {
            float dir = chooseDir(delta.z, workingVelocity.z);
            workingCenter.z += dir * pushDistance;

            if ((dir > 0.0f && workingVelocity.z < 0.0f) ||
                (dir < 0.0f && workingVelocity.z > 0.0f))
            {
                workingVelocity.z = 0.0f;
            }
        }

        return true;
    }

    // --- Full collision resolution step ---

    void Rigidbody::resolveCollisions(const std::vector<BoxCollider> &worldColliders)
    {
        // Previous and current centers (world-space).
        const Vec3 prevBottom = previousBottom;
        const Vec3 prevCenter = prevBottom + collider.center;
        const Vec3 currBottom = transform.position;
        const Vec3 currCenter = currBottom + collider.center;

        Vec3 remainingMotion = currCenter - prevCenter;
        Vec3 workingCenter = prevCenter;
        Vec3 workingVelocity = velocity;
        bool groundedFlag = false;

        // No motion: just resolve static penetration.
        if (length_sq(remainingMotion) < 1e-8f)
        {
            for (int iter = 0; iter < 4; ++iter)
            {
                bool corrected = false;
                for (const auto &box : worldColliders)
                    corrected |= resolvePenetrationOne(box, workingCenter, workingVelocity, groundedFlag);
                if (!corrected)
                    break;
            }

            const Vec3 finalCenter = workingCenter;
            const Vec3 finalBottom = finalCenter - collider.center;

            transform.position = finalBottom;
            velocity = workingVelocity;
            grounded = groundedFlag;
            return;
        }

        // Swept collision iterations.
        for (int iteration = 0;
             iteration < 4 && length_sq(remainingMotion) > 1e-8f;
             ++iteration)
        {
            float bestTime = 1.0f;
            Vec3 bestNormal{0.0f, 0.0f, 0.0f};
            bool hitFound = false;

            auto considerCollider = [&](const BoxCollider &box)
            {
                float hitTime = 0.0f;
                Vec3 hitNormal{0.0f, 0.0f, 0.0f};
                if (sweepAgainstBox(box, workingCenter, remainingMotion, hitTime, hitNormal))
                {
                    if (hitTime < bestTime)
                    {
                        bestTime = hitTime;
                        bestNormal = hitNormal;
                        hitFound = true;
                    }
                }
            };

            for (const auto &box : worldColliders)
                considerCollider(box);

            if (!hitFound)
            {
                // No collisions: move full remaining motion.
                workingCenter += remainingMotion;
                remainingMotion = {0.0f, 0.0f, 0.0f};
                break;
            }

            // Move up to just before the hit.
            const float advance = std::max(0.0f, bestTime - RAY_EPSILON);
            workingCenter += remainingMotion * advance;

            // Clip velocity along collision normal.
            const float velAlongNormal = dot(workingVelocity, bestNormal);
            if (velAlongNormal < 0.0f)
                workingVelocity -= bestNormal * velAlongNormal;

            // Slide remaining motion along the surface.
            float remainingFraction = 1.0f - bestTime;
            remainingFraction = std::clamp(remainingFraction, 0.0f, 1.0f);
            remainingMotion *= remainingFraction;

            const float motionAlongNormal = dot(remainingMotion, bestNormal);
            remainingMotion -= bestNormal * motionAlongNormal;

            if (bestNormal.y > 0.5f)
                groundedFlag = true;

            if (length_sq(remainingMotion) < 1e-8f)
            {
                remainingMotion = {0.0f, 0.0f, 0.0f};
                break;
            }
        }

        // Apply any remaining motion.
        workingCenter += remainingMotion;

        // Final penetration cleanup pass.
        for (int iter = 0; iter < 4; ++iter)
        {
            bool corrected = false;
            for (const auto &box : worldColliders)
                corrected |= resolvePenetrationOne(box, workingCenter, workingVelocity, groundedFlag);
            if (!corrected)
                break;
        }

        const Vec3 finalCenter = workingCenter;
        const Vec3 finalBottom = finalCenter - collider.center;

        transform.position = finalBottom;
        velocity = workingVelocity;
        grounded = groundedFlag;
    }

    // --- Single-call physics step ---

    void Rigidbody::update(float dt, const std::vector<BoxCollider> &worldColliders)
    {
        beginStep();
        integrate(dt);
        resolveCollisions(worldColliders);
    }

} // namespace mcu_game
