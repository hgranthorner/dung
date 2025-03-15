#pragma once

#include <SDL3/SDL.h>

#include <stdlib.h>

void load_shaders(SDL_GPUDevice *device, const char *filename, SDL_GPUShader **dest);
void map_buffer(SDL_GPUDevice *device, SDL_GPUBuffer *buffer, void *dest, size_t size);
