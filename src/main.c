#include "SDL3/SDL_gpu.h"
#include "SDL3/SDL_scancode.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <cglm/cglm.h>

#include <SDL3/SDL.h>
#include <SDL3/SDL_video.h>

#include "camera.h"
#include "constants.h"
#include "pipeline.h"
#include "sdl_utils.h"

int main() {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        fprintf(stderr, "Failed to init video! %s", SDL_GetError());
        return 1;
    };

    SDL_Window *window = NULL;
    window = SDL_CreateWindow("Dung", SCREEN_WIDTH, SCREEN_HEIGHT, 0);
    CHECK(window);

    SDL_GPUDevice *device = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_MSL, true, NULL);
    CHECK(device);

    const char *device_driver = SDL_GetGPUDeviceDriver(device);
    CHECK(device_driver);

    CHECK(SDL_ClaimWindowForGPUDevice(device, window));

    Pipeline pipeline;
    renderer_init(&pipeline, window, device);

    VectorInput *vertex_data = malloc(VerticesSize);
    map_buffer(device, pipeline.vertex_buffer, vertex_data, VerticesSize);
    for (size_t i = 0; i < VerticesCount; i++) {
        printf("Vertex #%zu: x=%f, y=%f\n", i, vertex_data[i].position[0], vertex_data[i].position[1]);
    }
    free(vertex_data);

    uint16_t *indices_data = malloc(IndicesSize);
    map_buffer(device, pipeline.index_buffer, indices_data, IndicesSize);
    for (size_t i = 0; i < IndicesCount; i++) {
        printf("Index #%zu: %d\n", i, indices_data[i]);
    }
    free(indices_data);

    // finish loading data

    Camera camera = {0};
    camera_init(&camera);
    bool running = true;
    SDL_Event e;
    uint64_t now, previous, last_frame_time = SDL_GetTicks();

    while (running) {
        uint64_t ts = now - previous;

        while (SDL_PollEvent(&e)) {
            switch (e.type) {
            case SDL_EVENT_KEY_DOWN: {
                switch (e.key.scancode) {
                case SDL_SCANCODE_ESCAPE: {
                    running = false;
                } break;

                case SDL_SCANCODE_W:
                case SDL_SCANCODE_UP: {
                    camera_zoom(&camera, CAMERA_ZOOM_OUT, ts);
                } break;

                case SDL_SCANCODE_S:
                case SDL_SCANCODE_DOWN: {
                    camera_zoom(&camera, CAMERA_ZOOM_IN, ts);
                } break;

                case SDL_SCANCODE_A: {
                    camera_rotate_around_point(&camera, camera.target, CAMERA_DIRECTION_LEFT, ts);
                } break;

                case SDL_SCANCODE_D: {
                    camera_rotate_around_point(&camera, camera.target, CAMERA_DIRECTION_RIGHT, ts);
                } break;

                case SDL_SCANCODE_Q: {
                    camera_strafe(&camera, CAMERA_DIRECTION_LEFT, ts);
                } break;

                case SDL_SCANCODE_E: {
                    camera_strafe(&camera, CAMERA_DIRECTION_RIGHT, ts);
                } break;

                default:
                    continue;
                }
            } break;
            case SDL_EVENT_QUIT: {
                running = false;
            } break;
            }
        }

        previous = now;
        now = SDL_GetTicks();
        if (now - last_frame_time >= SCREEN_TICKS_PER_FRAME) {
            last_frame_time = now;

            SDL_GPUCommandBuffer *cmdbuf = SDL_AcquireGPUCommandBuffer(device);
            if (cmdbuf == NULL) {
                fprintf(stderr, "ERROR: SDL_AcquireGPUCommandBuffer failed: %s\n", SDL_GetError());
                break;
            }

            SDL_GPUTexture *swapchain_texture;
            if (!SDL_WaitAndAcquireGPUSwapchainTexture(cmdbuf, window, &swapchain_texture, NULL, NULL)) {
                fprintf(stderr, "ERROR: SDL_WaitAndAcquireGPUSwapchainTexture failed: %s\n", SDL_GetError());
                break;
            }

            if (swapchain_texture == NULL) {
                fprintf(stderr, "ERROR: swapchain_texture is NULL\n");
                SDL_SubmitGPUCommandBuffer(cmdbuf);
                break;
            }

            SDL_GPUColorTargetInfo color_target_info = {0};
            color_target_info.texture = swapchain_texture;
            color_target_info.clear_color = COLOR_BLACK;
            color_target_info.load_op = SDL_GPU_LOADOP_CLEAR;
            color_target_info.store_op = SDL_GPU_STOREOP_STORE;

            SDL_GPURenderPass *render_pass = SDL_BeginGPURenderPass(cmdbuf, &color_target_info, 1, NULL);
            CHECK(render_pass);

            SDL_PushGPUVertexUniformData(cmdbuf, 0, camera.mvp, sizeof(mat4));
            SDL_BindGPUGraphicsPipeline(render_pass, pipeline.pipeline);
            SDL_BindGPUVertexBuffers(render_pass, 0,
                                     &(SDL_GPUBufferBinding){.buffer = pipeline.vertex_buffer, .offset = 0}, 1);
            SDL_BindGPUIndexBuffer(render_pass, &(SDL_GPUBufferBinding){.buffer = pipeline.index_buffer, .offset = 0},
                                   SDL_GPU_INDEXELEMENTSIZE_16BIT);
            SDL_DrawGPUIndexedPrimitives(render_pass, IndicesCount, IndicesCount, 0, 0, 0);
            SDL_EndGPURenderPass(render_pass);

            CHECK(SDL_SubmitGPUCommandBuffer(cmdbuf));
        }
    }
    return 0;
}
