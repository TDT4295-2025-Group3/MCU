#include "host_sevenseg.hpp"

#include <SDL3/SDL.h>
#include <string>
#include <cmath>

HostSevenSeg::HostSevenSeg() {
    SDL_Init(SDL_INIT_VIDEO);

    window = SDL_CreateWindow(
        "Upwards (Simulator) - 7-Segment Display",
        400,
        150,
        SDL_WINDOW_RESIZABLE
    );

    renderer = SDL_CreateRenderer(window, nullptr);

    // Clear to black initially
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
    SDL_RenderPresent(renderer);
}

HostSevenSeg::~HostSevenSeg() {
    if (renderer) SDL_DestroyRenderer(renderer);
    if (window) SDL_DestroyWindow(window);
    SDL_Quit();
}

void HostSevenSeg::setDisplayedValue(std::string value) {
    // Clear screen
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    // Set red color for segments
    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);

    // Draw each character
    float x = 20.0f;
    float y = 30.0f;
    float segWidth = 40.0f;
    float segHeight = 70.0f;
    float charSpacing = 60.0f;

    for (char c : value) {
        drawChar(c, x, y, segWidth, segHeight);
        x += charSpacing;
    }

    SDL_RenderPresent(renderer);

    // Process events
    SDL_Event event;
    while (SDL_PollEvent(&event)) {}
}

void HostSevenSeg::drawChar(char c, float x, float y, float width, float height) {
    // Seven segment mapping:
    //   aaa
    //  f   b
    //   ggg
    //  e   c
    //   ddd

    // Segment patterns for each character (a, b, c, d, e, f, g)
    const bool patterns[11][7] = {
        {1, 1, 1, 1, 1, 1, 0}, // 0
        {0, 1, 1, 0, 0, 0, 0}, // 1
        {1, 1, 0, 1, 1, 0, 1}, // 2
        {1, 1, 1, 1, 0, 0, 1}, // 3
        {0, 1, 1, 0, 0, 1, 1}, // 4
        {1, 0, 1, 1, 0, 1, 1}, // 5
        {1, 0, 1, 1, 1, 1, 1}, // 6
        {1, 1, 1, 0, 0, 0, 0}, // 7
        {1, 1, 1, 1, 1, 1, 1}, // 8
        {1, 1, 1, 1, 0, 1, 1}, // 9
        {0, 0, 0, 0, 0, 0, 1}, // - (minus)
    };

    int index = -1;
    if (c >= '0' && c <= '9') {
        index = c - '0';
    } else if (c == '-') {
        index = 10;
    }

    if (index == -1) return; // Skip unknown characters

    float segThickness = width * 0.15f;
    float halfHeight = height / 2.0f;
    float gap = segThickness * 0.3f; // Small gap between segments

    // Draw segment a (top horizontal)
    if (patterns[index][0]) {
        drawHorizontalSegment(x + gap, y, width - gap * 2, segThickness);
    }

    // Draw segment b (top right vertical)
    if (patterns[index][1]) {
        drawVerticalSegment(x + width - segThickness, y + gap, halfHeight - gap * 1.5f, segThickness);
    }

    // Draw segment c (bottom right vertical)
    if (patterns[index][2]) {
        drawVerticalSegment(x + width - segThickness, y + halfHeight + gap * 0.5f, halfHeight - gap * 1.5f, segThickness);
    }

    // Draw segment d (bottom horizontal)
    if (patterns[index][3]) {
        drawHorizontalSegment(x + gap, y + height - segThickness, width - gap * 2, segThickness);
    }

    // Draw segment e (bottom left vertical)
    if (patterns[index][4]) {
        drawVerticalSegment(x, y + halfHeight + gap * 0.5f, halfHeight - gap * 1.5f, segThickness);
    }

    // Draw segment f (top left vertical)
    if (patterns[index][5]) {
        drawVerticalSegment(x, y + gap, halfHeight - gap * 1.5f, segThickness);
    }

    // Draw segment g (middle horizontal)
    if (patterns[index][6]) {
        drawHorizontalSegment(x + gap, y + halfHeight - segThickness / 2, width - gap * 2, segThickness);
    }
}

void HostSevenSeg::drawHorizontalSegment(float x, float y, float width, float thickness) {
    // Draw trapezoid-like shape using multiple rectangles for smooth appearance
    float taper = thickness * 0.4f; // Amount of tapering on the ends

    // Main rectangle
    SDL_FRect mainRect = {x + taper, y, width - taper * 2, thickness};
    SDL_RenderFillRect(renderer, &mainRect);

    // Left taper
    for (int i = 0; i < (int)taper; i++) {
        float ratio = (float)i / taper;
        float h = thickness * ratio;
        SDL_FRect rect = {x + i, y + (thickness - h) / 2, 1, h};
        SDL_RenderFillRect(renderer, &rect);
    }

    // Right taper
    for (int i = 0; i < (int)taper; i++) {
        float ratio = (float)i / taper;
        float h = thickness * (1.0f - ratio);
        SDL_FRect rect = {x + width - i - 1, y + (thickness - h) / 2, 1, h};
        SDL_RenderFillRect(renderer, &rect);
    }
}

void HostSevenSeg::drawVerticalSegment(float x, float y, float height, float thickness) {
    // Draw trapezoid-like shape using multiple rectangles for smooth appearance
    float taper = thickness * 0.4f; // Amount of tapering on the ends

    // Main rectangle
    SDL_FRect mainRect = {x, y + taper, thickness, height - taper * 2};
    SDL_RenderFillRect(renderer, &mainRect);

    // Top taper
    for (int i = 0; i < (int)taper; i++) {
        float ratio = (float)i / taper;
        float w = thickness * ratio;
        SDL_FRect rect = {x + (thickness - w) / 2, y + i, w, 1};
        SDL_RenderFillRect(renderer, &rect);
    }

    // Bottom taper
    for (int i = 0; i < (int)taper; i++) {
        float ratio = (float)i / taper;
        float w = thickness * (1.0f - ratio);
        SDL_FRect rect = {x + (thickness - w) / 2, y + height - i - 1, w, 1};
        SDL_RenderFillRect(renderer, &rect);
    }
}