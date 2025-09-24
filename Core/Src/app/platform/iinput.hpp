#pragma once

struct KeyState {
    bool up{}, down{}, left{}, right{}, a{}, b{};
};

class IInput {
    // Interface for input
public:
    virtual ~IInput() = default;
    virtual KeyState poll() = 0;
};