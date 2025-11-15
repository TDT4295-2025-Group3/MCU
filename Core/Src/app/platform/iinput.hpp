#pragma once

struct KeyState {
    // Arrow keys for player movement (camera-relative motion handled in game logic)
    // x: left (-1) to right (+1), y: down (-1) to up (+1)
    float x{}, y{};

    // Camera look controls (digital).
    // cam_x: yaw left/right, cam_y: pitch up/down
    float cam_x{}, cam_y{};

    // Jump
    bool space{};
};

class IInput {
    // Interface for input
public:
    virtual ~IInput() = default;
    virtual KeyState poll() = 0;
};