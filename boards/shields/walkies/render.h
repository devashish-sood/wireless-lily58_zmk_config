/* SPDX-License-Identifier: MIT */
#pragma once
#include <stdbool.h>
#include <stdint.h>

#define WALKIES_WIDTH 68
#define WALKIES_HEIGHT 160
#define WALKIES_BYTES ((WALKIES_WIDTH * WALKIES_HEIGHT) / 8)

struct walkies_state {
    uint8_t battery;
    uint8_t layer;
    uint8_t wpm;
    uint8_t rest_frame;
    bool usb;
    bool connected;
    bool paired;
    bool moving;
    uint32_t step;
};

void walkies_render(uint8_t image[WALKIES_BYTES], const struct walkies_state *state,
                    bool central);
bool walkies_pixel(const uint8_t image[WALKIES_BYTES], unsigned x, unsigned y);
