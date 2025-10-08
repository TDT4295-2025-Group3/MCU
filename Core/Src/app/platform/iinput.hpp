#pragma once

struct KeyState {
    // Arrow keys for player movement (camera-relative motion handled in game logic)
    bool up{}, down{}, left{}, right{};

    // Camera look controls (digital).
    // WASD: W/S = pitch up/down, A/D = yaw left/right
    bool w{}, a{}, s{}, d{};

    // Jump
    bool space{};
};

class IInput {
    // Interface for input
public:
    virtual ~IInput() = default;
    virtual KeyState poll() = 0;
};