#pragma once
#include <chrono>

#include "iinput.hpp"
#include "itimer.hpp"
#include "hid_driver.hpp"


class HostTimer : public ITimer {
public:
    uint32_t get_ticks_ms() override;
};

class HostInput : public IInput {
public:
    KeyState poll() override;
};

class DS4Input : public IInput {
public:
    explicit DS4Input(DS4Driver& controller) : controller(controller) {}
    KeyState poll() override;

private:
    DS4Driver& controller;
};