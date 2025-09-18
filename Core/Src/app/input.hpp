#pragma once

// Simple placeholder input abstraction. Replace with actual HW reads later.
namespace mcu_game {

struct InputState {
    // Movement input in local camera-space projected onto XZ plane.
    // Expected range [-1,1]. For digital buttons use { -1,0,1 }.
    float moveX{0}; // strafe (right +)
    float moveZ{0}; // forward (towards camera forward projection +)
    bool jump{false};

    // Camera look deltas (radians per tick or accumulated value to add).
    float lookYawDelta{0};
    float lookPitchDelta{0};
};

} // namespace mcu_game
