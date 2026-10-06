/* SPDX-License-Identifier: MIT */
#pragma once
#include <stdbool.h>
#include <stdint.h>

/* A missing/stale relay update always resolves to the resting pose. */
bool walkies_typing(uint8_t *wpm);
