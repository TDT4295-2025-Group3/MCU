#pragma once

#include <cmath>
#include <cstdint>

namespace mcu_game
{

    struct Vec2
    {
        float x{0}, y{0};

        constexpr Vec2() = default;
        constexpr Vec2(float x_, float y_) : x(x_), y(y_) {}
    };

    /*********************************************************************************************
     * @brief Simple 3D vector (x, y, z) with basic arithmetic and scaling operations.
     *
     * This struct represents a 3D vector using single-precision floating point components.
     * It provides constexpr-capable constructors and value-semantic arithmetic operators
     * for efficient use in math-heavy code (e.g. physics, camera, movement).
     *
     * Coordinate system convention:
     *   X: right
     *   Y: up
     *   Z: forward
     *
     * @warning Division operations do not guard against division by zero;
     *          ensure scalar != 0 when using / or /=.
     *
     * @see dot(), cross(), length(), normalize(), lerp()
     *********************************************************************************************/
    struct Vec3
    {
        float x{0}, y{0}, z{0};

        constexpr Vec3() = default;
        constexpr Vec3(float x_, float y_, float z_) : x(x_), y(y_), z(z_) {}

        constexpr Vec3 operator+(const Vec3 &o) const { return {x + o.x, y + o.y, z + o.z}; }
        constexpr Vec3 operator-(const Vec3 &o) const { return {x - o.x, y - o.y, z - o.z}; }
        constexpr Vec3 operator*(float s) const { return {x * s, y * s, z * s}; }
        constexpr Vec3 operator/(float s) const { return {x / s, y / s, z / s}; }
        Vec3 &operator+=(const Vec3 &o)
        {
            x += o.x;
            y += o.y;
            z += o.z;
            return *this;
        }
        Vec3 &operator-=(const Vec3 &o)
        {
            x -= o.x;
            y -= o.y;
            z -= o.z;
            return *this;
        }
        Vec3 &operator*=(float s)
        {
            x *= s;
            y *= s;
            z *= s;
            return *this;
        }
        Vec3 &operator/=(float s)
        {
            x /= s;
            y /= s;
            z /= s;
            return *this;
        }
    };

    // scalar * vector
    inline constexpr Vec3 operator*(float s, const Vec3 &v) { return v * s; }

    // useful vector operations
    inline float dot(const Vec3 &a, const Vec3 &b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
    inline Vec3 cross(const Vec3 &a, const Vec3 &b) { return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x}; }
    inline float length_sq(const Vec3 &v) { return dot(v, v); }
    inline float length(const Vec3 &v) { return std::sqrt(length_sq(v)); }
    inline Vec3 normalize(const Vec3 &v)
    {
        float l = length(v);
        if (l <= 1e-6f)
            return {0, 0, 0}; // avoid division by zero
        return v / l;
    }

    // linear interpolation (for smoothing stuff)
    inline Vec3 lerp(const Vec3 &a, const Vec3 &b, float t) { return a + (b - a) * t; }
    inline float lerp(float a, float b, float t) { return a + (b - a) * t; }

    /*********************************************************************************************
     * @brief Builds a forward vector from yaw/pitch angles.
     * @param yaw The yaw angle (rotation around the Y-axis) in radians.
     * @param pitch The pitch angle (rotation around the X-axis) in radians.
     * @return A Vec3 representing the forward direction pointing in the
     *         direction the player/camera is facing.
     *********************************************************************************************/
    inline Vec3 forward_vector_from_yaw_pitch(float yaw, float pitch)
    {
        float cy = std::cos(yaw);
        float sy = std::sin(yaw);
        float cp = std::cos(pitch);
        float sp = std::sin(pitch);
        return {-sy * cp, sp, cy * cp};
    }

} // namespace mcu_game
