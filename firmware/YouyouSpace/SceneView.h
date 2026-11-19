#pragma once
#include <lvgl.h>
#include "Game.h"
// Three buffers live in PSRAM on the board, ordinary RAM in the native harness.
bool viewInit(void *pixelBuffer, void *backgroundBuffer, void *previousBuffer);
void viewDraw(const Game &game, bool tiltAvailable);
lv_obj_t *viewCanvas();
struct ViewStats { unsigned pixels=0,spans=0,staticBuilds=0; };
ViewStats viewStats();
void viewInvalidateCache(); // Native verification: compare incremental and full composition.
