#pragma once

#include <SDL3/SDL.h>
#include <SDL3/SDL_gpu.h>
#include <cglm/cglm.h>

typedef struct {
    vec4 position, color;
} VectorInput;

typedef struct {
    SDL_GPUGraphicsPipeline *pipeline;
    SDL_GPUBuffer *vertex_buffer;
    SDL_GPUBuffer *index_buffer;
} Pipeline;

const VectorInput Vertices[8];
const size_t VerticesCount;
const size_t VerticesSize;

// clang-format off
const uint16_t Indices[24];
// clang-format on
const size_t IndicesCount;
const size_t IndicesSize;

void renderer_init(Pipeline *pipeline, SDL_Window *window, SDL_GPUDevice *device);
