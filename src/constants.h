#pragma once

#include <SDL3/SDL.h>

const int SCREEN_WIDTH;
const int SCREEN_HEIGHT;
const int SCREEN_FPS;
const int SCREEN_TICKS_PER_FRAME;

const SDL_FColor COLOR_WHITE;
const SDL_FColor COLOR_BLACK;
const SDL_FColor COLOR_RED;
const SDL_FColor COLOR_GREEN;
const SDL_FColor COLOR_BLUE;
const SDL_FColor COLOR_CYAN;
const SDL_FColor COLOR_YELLOW;
const SDL_FColor COLOR_PINK;

#define CHECK(x)                                                                                                       \
    do {                                                                                                               \
        if (!x) {                                                                                                      \
            fprintf(stderr, "SDL Error: %s", SDL_GetError());                                                          \
            exit(1);                                                                                                   \
        }                                                                                                              \
    } while (0)
