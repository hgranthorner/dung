#pragma once

#include <SDL3/SDL.h>
#include <SDL3/SDL_gpu.h>
#include <cglm/cglm.h>

typedef struct {
    vec4 position, color;
} Vertex;

typedef struct {
    SDL_GPUGraphicsPipeline *pipeline;
    SDL_GPUBuffer *vertex_buffer;
    SDL_GPUBuffer *index_buffer;
    Vertex *vertices;
    size_t vertices_count;
    uint16_t *indices;
    size_t indices_count;
} Pipeline;

void cube_pipeline_init(Pipeline *pipeline, SDL_Window *window, SDL_GPUDevice *device);
void floor_tile_pipeline_init(Pipeline *pipeline, SDL_Window *window, SDL_GPUDevice *device);
void pipeline_render(Pipeline *pipeline, SDL_GPURenderPass *render_pass);
