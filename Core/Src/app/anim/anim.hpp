#pragma once
#include <cstdint>
#include <vector>
#include <cstddef>
#include <string>
#include "platform/irasterizer.hpp"

struct AnimState
{
    uint32_t vertexId = 0;
    uint32_t triangleId = 0;
    Rasterizer::Transform transform;
};

struct Keyframe
{
    bool useModelSwap = false;
    uint32_t vertexId = 0;
    uint32_t triangleId = 0;

    bool useTranslation = false;
    float translationX = 0.0f;
    float translationY = 0.0f;
    float translationZ = 0.0f;

    bool useRotation = false;
    float rotationX = 0.0f;
    float rotationY = 0.0f;
    float rotationZ = 0.0f;

    bool useScale = false;
    float scaleX = 1.0f;
    float scaleY = 1.0f;
    float scaleZ = 1.0f;

    float duration = 0.0f; // duration *after* this key until the next one
};

struct Animation
{
    std::string name;
    std::vector<Keyframe> keyframes;
    bool loop = true;
};

class Animator
{
public:
    Animator() = default;

    void addAnimation(const Animation &anim)
    {
        animations.push_back(anim);
    }

    void playAnimation(const std::string &name)
    {
        if (name == animations[currentAnimationIndex].name)
            return; // already playing

        for (std::size_t i = 0; i < animations.size(); ++i)
        {
            if (animations[i].name == name)
            {
                currentAnimationIndex = i;
                currentTime = 0.0f;
                rebuildTimeline();
                playing = true;
                return;
            }
        }
        // Not found -> stop playing
        playing = false;
        currentAnimationIndex = npos;
    }

    void update(float deltaTime)
    {
        if (!playing || currentAnimationIndex == npos)
            return;

        const Animation &anim = animations[currentAnimationIndex];
        if (anim.keyframes.empty() || totalDuration <= 0.0f)
            return;

        currentTime += deltaTime;

        if (anim.loop)
        {
            // Wrap time in [0, totalDuration)
            while (currentTime >= totalDuration)
                currentTime -= totalDuration;
            while (currentTime < 0.0f)
                currentTime += totalDuration;
        }
        else
        {
            if (currentTime < 0.0f)
                currentTime = 0.0f;
            if (currentTime > totalDuration)
                currentTime = totalDuration;
        }
    }

    AnimState getCurrentAnimState(uint32_t baseVertexId, uint32_t baseTriangleId, const Rasterizer::Transform &baseTransform)
    {
        if (currentAnimationIndex == npos ||
            currentAnimationIndex >= animations.size() ||
            animations[currentAnimationIndex].keyframes.empty() ||
            totalDuration <= 0.0f)
        {
            return AnimState{
                baseVertexId,
                baseTriangleId,
                baseTransform};
        }

        const Animation &anim = animations[currentAnimationIndex];
        Keyframe keyFrame = sampleAtTime(anim, currentTime);
        Rasterizer::Transform animTransform = baseTransform;
        if (keyFrame.useTranslation)
        {
            animTransform.position.x += keyFrame.translationX;
            animTransform.position.y += keyFrame.translationY;
            animTransform.position.z += keyFrame.translationZ;
        }
        if (keyFrame.useRotation)
        {
            animTransform.rotation.x += keyFrame.rotationX;
            animTransform.rotation.y += keyFrame.rotationY;
            animTransform.rotation.z += keyFrame.rotationZ;
        }
        if (keyFrame.useScale)
        {
            animTransform.scale.x *= keyFrame.scaleX;
            animTransform.scale.y *= keyFrame.scaleY;
            animTransform.scale.z *= keyFrame.scaleZ;
        }

        if (keyFrame.useModelSwap)
        {
            baseVertexId = keyFrame.vertexId;
            baseTriangleId = keyFrame.triangleId;
        }

        return AnimState{
            baseVertexId,
            baseTriangleId,
            animTransform};
    }

private:
    static constexpr std::size_t npos = static_cast<std::size_t>(-1);

    std::vector<Animation> animations;
    std::size_t currentAnimationIndex = npos;
    float currentTime = 0.0f;
    float totalDuration = 0.0f;
    bool playing = false;

    // timeline: keyTimes[i] is the absolute time (from start of anim)
    // at which keyframe i starts.
    std::vector<float> keyTimes;

    void rebuildTimeline()
    {
        totalDuration = 0.0f;
        keyTimes.clear();

        if (currentAnimationIndex == npos || currentAnimationIndex >= animations.size())
            return;

        const Animation &anim = animations[currentAnimationIndex];
        keyTimes.resize(anim.keyframes.size());
        float t = 0.0f;
        for (std::size_t i = 0; i < anim.keyframes.size(); ++i)
        {
            keyTimes[i] = t;
            t += anim.keyframes[i].duration;
        }
        totalDuration = (t > 0.0f ? t : 0.0f);
    }

    Keyframe sampleAtTime(const Animation &anim, float t) const
    {
        Keyframe result{};

        const auto &kfs = anim.keyframes;
        if (kfs.empty())
            return result;

        // Clamp local sampling time
        if (t < 0.0f)
            t = 0.0f;
        if (t > totalDuration)
            t = totalDuration;

        // Helper: find prev/next keyframe indices that have the given "use" flag set.
        auto findPrevNext = [&](bool Keyframe::*useFlag) -> std::pair<int, int>
        {
            int prev = -1;
            int next = -1;

            for (std::size_t i = 0; i < kfs.size(); ++i)
            {
                if (!(kfs[i].*useFlag))
                    continue;

                float kt = keyTimes[i];

                if (kt <= t)
                {
                    if (prev == -1 || kt >= keyTimes[static_cast<std::size_t>(prev)])
                        prev = static_cast<int>(i);
                }

                if (kt >= t)
                {
                    if (next == -1 || kt <= keyTimes[static_cast<std::size_t>(next)])
                        next = static_cast<int>(i);
                }
            }

            if (prev == -1 && next == -1)
                return {-1, -1};

            if (prev == -1)
                prev = next;
            if (next == -1)
                next = prev;
            return {prev, next};
        };

        auto lerp = [](float a, float b, float alpha) -> float
        {
            return a + (b - a) * alpha;
        };

        // --- Translation ---
        {
            auto [prevIdx, nextIdx] = findPrevNext(&Keyframe::useTranslation);
            if (prevIdx == -1 && nextIdx == -1)
            {
                // No translation keys at all: already identity (0,0,0)
                result.useTranslation = true;
            }
            else
            {
                result.useTranslation = true;
                const Keyframe &kf0 = kfs[static_cast<std::size_t>(prevIdx)];
                const Keyframe &kf1 = kfs[static_cast<std::size_t>(nextIdx)];

                float alpha = 0.0f;
                if (prevIdx != nextIdx)
                {
                    float t0 = keyTimes[static_cast<std::size_t>(prevIdx)];
                    float t1 = keyTimes[static_cast<std::size_t>(nextIdx)];
                    float span = t1 - t0;
                    if (span > 0.0f)
                    {
                        alpha = (t - t0) / span;
                        if (alpha < 0.0f)
                            alpha = 0.0f;
                        if (alpha > 1.0f)
                            alpha = 1.0f;
                    }
                }

                result.translationX = lerp(kf0.translationX, kf1.translationX, alpha);
                result.translationY = lerp(kf0.translationY, kf1.translationY, alpha);
                result.translationZ = lerp(kf0.translationZ, kf1.translationZ, alpha);
            }
        }

        // --- Rotation ---
        {
            auto [prevIdx, nextIdx] = findPrevNext(&Keyframe::useRotation);
            if (prevIdx == -1 && nextIdx == -1)
            {
                // No rotation keys: identity (0,0,0)
                result.useRotation = true;
            }
            else
            {
                result.useRotation = true;
                const Keyframe &kf0 = kfs[static_cast<std::size_t>(prevIdx)];
                const Keyframe &kf1 = kfs[static_cast<std::size_t>(nextIdx)];

                float alpha = 0.0f;
                if (prevIdx != nextIdx)
                {
                    float t0 = keyTimes[static_cast<std::size_t>(prevIdx)];
                    float t1 = keyTimes[static_cast<std::size_t>(nextIdx)];
                    float span = t1 - t0;
                    if (span > 0.0f)
                    {
                        alpha = (t - t0) / span;
                        if (alpha < 0.0f)
                            alpha = 0.0f;
                        if (alpha > 1.0f)
                            alpha = 1.0f;
                    }
                }

                // Simple linear interpolation of Euler angles.
                // (You can later improve with proper wrap-around/slerp if needed.)
                result.rotationX = lerp(kf0.rotationX, kf1.rotationX, alpha);
                result.rotationY = lerp(kf0.rotationY, kf1.rotationY, alpha);
                result.rotationZ = lerp(kf0.rotationZ, kf1.rotationZ, alpha);
            }
        }

        // --- Scale ---
        {
            auto [prevIdx, nextIdx] = findPrevNext(&Keyframe::useScale);
            if (prevIdx == -1 && nextIdx == -1)
            {
                // No scale keys: identity (1,1,1)
                result.useScale = true;
                result.scaleX = 1.0f;
                result.scaleY = 1.0f;
                result.scaleZ = 1.0f;
            }
            else
            {
                result.useScale = true;
                const Keyframe &kf0 = kfs[static_cast<std::size_t>(prevIdx)];
                const Keyframe &kf1 = kfs[static_cast<std::size_t>(nextIdx)];

                float alpha = 0.0f;
                if (prevIdx != nextIdx)
                {
                    float t0 = keyTimes[static_cast<std::size_t>(prevIdx)];
                    float t1 = keyTimes[static_cast<std::size_t>(nextIdx)];
                    float span = t1 - t0;
                    if (span > 0.0f)
                    {
                        alpha = (t - t0) / span;
                        if (alpha < 0.0f)
                            alpha = 0.0f;
                        if (alpha > 1.0f)
                            alpha = 1.0f;
                    }
                }

                result.scaleX = lerp(kf0.scaleX, kf1.scaleX, alpha);
                result.scaleY = lerp(kf0.scaleY, kf1.scaleY, alpha);
                result.scaleZ = lerp(kf0.scaleZ, kf1.scaleZ, alpha);
            }
        }

        // --- Model swap (no interpolation, step) ---
        {
            int chosen = -1;
            // find last useModelSwap keyframe at or before time t
            for (std::size_t i = 0; i < kfs.size(); ++i)
            {
                if (!kfs[i].useModelSwap)
                    continue;

                float kt = keyTimes[i];
                if (kt <= t)
                {
                    if (chosen == -1 || kt >= keyTimes[static_cast<std::size_t>(chosen)])
                        chosen = static_cast<int>(i);
                }
            }

            // If none at or before t, optionally pick the first one after t
            if (chosen == -1)
            {
                for (std::size_t i = 0; i < kfs.size(); ++i)
                {
                    if (kfs[i].useModelSwap)
                    {
                        chosen = static_cast<int>(i);
                        break;
                    }
                }
            }

            if (chosen != -1)
            {
                const Keyframe &kf = kfs[static_cast<std::size_t>(chosen)];
                result.useModelSwap = true;
                result.vertexId = kf.vertexId;
                result.triangleId = kf.triangleId;
            }
            else
            {
                // No model swap keys -> leave as identity (no swap)
                result.useModelSwap = false;
            }
        }

        return result;
    }
};
