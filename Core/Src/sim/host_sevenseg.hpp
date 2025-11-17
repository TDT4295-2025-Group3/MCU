#pragma once
#include <SDL3/SDL_render.h>
#include <SDL3/SDL_video.h>

#include "isevenseg.hpp"

/**
 * Host (simulation) implementation of ISevenSeg.
 * Seven segment display is simulated using a SDL window.
 */
class HostSevenSeg : public ISevenSeg {
public:
    HostSevenSeg();
    ~HostSevenSeg() override;

    void setDisplayedValue(std::string value) override;

private:
    SDL_Window* window;
    SDL_Renderer* renderer;

    void drawChar(char c, float x, float y, float width, float height);
    void drawHorizontalSegment(float x, float y, float width, float thickness);
    void drawVerticalSegment(float x, float y, float height, float thickness);
};