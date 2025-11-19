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
    void setRumble(float x) override;
    void setInputColor(int r, int g, int b) override;
    void setBlinking(int interval_on_ms, int interval_off_ms) override;
};

class DS4Input : public IInput {
public:
    explicit DS4Input(DS4Driver& driver) : controller(driver) {}
    KeyState poll() override;
    void setRumble(float x) override;
    void setInputColor(int r, int g, int b) override;
    void setBlinking(int interval_on_ms, int interval_off_ms) override;

private:
    DS4Driver& controller;
};