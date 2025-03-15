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
#include "sdl_utils.h"

typedef struct {
    vec4 position, color;
} VectorInput;

const VectorInput Vertices[] = {
    // 0 fbl
    {{25, 25, 25, 1.0}, {1.0, 0.0, 0.0, 1.0}},
    // 1 ftl
    {{25, 75, 25, 1.0}, {0.0, 1.0, 0.0, 1.0}},
    // 2 fbr
    {{75, 25, 25, 1.0}, {0.0, 0.0, 1.0, 1.0}},
    // 3 ftr
    {{75, 75, 25, 1.0}, {1.0, 1.0, 0.0, 1.0}},

    // 4 bbr
    {{25, 25, -25, 1.0}, {1.0, 0.0, 0.0, 1.0}},
    // 5 btr
    {{25, 75, -25, 1.0}, {0.0, 1.0, 0.0, 1.0}},
    // 6 bbl
    {{75, 25, -25, 1.0}, {0.0, 0.0, 1.0, 1.0}},
    // 7 btl
    {{75, 75, -25, 1.0}, {1.0, 1.0, 0.0, 1.0}},
};
const size_t VerticesCount = sizeof(Vertices) / sizeof(VectorInput);
const size_t VerticesSize = sizeof(Vertices);

// clang-format off
const uint16_t Indices[] = {
    // front
    0, 1, 2,
    1, 3, 2,
    // right
    2, 3, 6,
    3, 7, 6,
    // left
    4, 5, 0,
    5, 1, 0,
    // back
    6, 7, 4,
    7, 5, 4,
};
// clang-format on
const size_t IndicesCount = sizeof(Indices) / sizeof(uint16_t);
const size_t IndicesSize = sizeof(Indices);

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

    SDL_GPUShader *shaders[2] = {0};
    load_shaders(device, "src/shader.metal", shaders);
    SDL_GPUShader *vert_shader = shaders[0];
    SDL_GPUShader *frag_shader = shaders[1];

    SDL_GPUGraphicsPipelineCreateInfo pipeline_info = {
        .target_info =
            {

                .num_color_targets = 1,
                .color_target_descriptions =
                    (SDL_GPUColorTargetDescription[]){{.format = SDL_GetGPUSwapchainTextureFormat(device, window)}},
            },
        .primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,
        .vertex_shader = vert_shader,
        .fragment_shader = frag_shader,
        .vertex_input_state =
            (SDL_GPUVertexInputState){
                .vertex_buffer_descriptions =
                    (SDL_GPUVertexBufferDescription[]){
                        {
                            .slot = 0,
                            .pitch = sizeof(VectorInput),
                            .input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX,
                            .instance_step_rate = 0,
                        },
                    },
                .num_vertex_buffers = 1,
                .vertex_attributes =
                    (SDL_GPUVertexAttribute[]){
                        {
                            .location = 0,
                            .buffer_slot = 0,
                            .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4,
                            .offset = 0,
                        },
                        {
                            .location = 1,
                            .buffer_slot = 0,
                            .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4,
                            .offset = sizeof(vec4),
                        },
                    },
                .num_vertex_attributes = 2,
            },
        .rasterizer_state =
            (SDL_GPURasterizerState){
                .cull_mode = SDL_GPU_CULLMODE_BACK,
                .front_face = SDL_GPU_FRONTFACE_CLOCKWISE,
                .fill_mode = SDL_GPU_FILLMODE_FILL,
            },
    };

    SDL_GPUGraphicsPipeline *pipeline = SDL_CreateGPUGraphicsPipeline(device, &pipeline_info);
    CHECK(pipeline);

    SDL_ReleaseGPUShader(device, vert_shader);
    SDL_ReleaseGPUShader(device, frag_shader);

    SDL_GPUBuffer *vertex_buffer = SDL_CreateGPUBuffer(
        device, &(SDL_GPUBufferCreateInfo){.usage = SDL_GPU_BUFFERUSAGE_VERTEX, .size = VerticesSize});
    CHECK(vertex_buffer);

    SDL_GPUBuffer *index_buffer = SDL_CreateGPUBuffer(
        device, &(SDL_GPUBufferCreateInfo){.usage = SDL_GPU_BUFFERUSAGE_INDEX, .size = IndicesSize});
    CHECK(index_buffer);

    // load static data to gpu
    {
        SDL_GPUTransferBuffer *transfer =
            SDL_CreateGPUTransferBuffer(device, &(SDL_GPUTransferBufferCreateInfo){
                                                    .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
                                                    .size = VerticesSize + IndicesSize,
                                                });
        VectorInput *vertex_data = SDL_MapGPUTransferBuffer(device, transfer, false);
        memcpy(vertex_data, Vertices, VerticesSize);
        uint16_t *index_data = (uint16_t *)&vertex_data[VerticesCount];
        memcpy(index_data, Indices, IndicesSize);

        SDL_UnmapGPUTransferBuffer(device, transfer);

        SDL_GPUCommandBuffer *upload_cmdbuf = SDL_AcquireGPUCommandBuffer(device);
        SDL_GPUCopyPass *copy_pass = SDL_BeginGPUCopyPass(upload_cmdbuf);

        SDL_UploadToGPUBuffer(copy_pass, &(SDL_GPUTransferBufferLocation){.transfer_buffer = transfer, .offset = 0},
                              &(SDL_GPUBufferRegion){.buffer = vertex_buffer, .offset = 0, .size = VerticesSize},
                              false);

        SDL_UploadToGPUBuffer(copy_pass,
                              &(SDL_GPUTransferBufferLocation){
                                  .transfer_buffer = transfer,
                                  .offset = VerticesSize,
                              },
                              &(SDL_GPUBufferRegion){
                                  .buffer = index_buffer,
                                  .offset = 0,
                                  .size = IndicesSize,
                              },
                              false);

        SDL_EndGPUCopyPass(copy_pass);
        SDL_GPUFence *fence = SDL_SubmitGPUCommandBufferAndAcquireFence(upload_cmdbuf);
        SDL_ReleaseGPUTransferBuffer(device, transfer);

        while (!SDL_QueryGPUFence(device, fence))
            ;

        SDL_ReleaseGPUFence(device, fence);
    }

    VectorInput *vertex_data = malloc(VerticesSize);
    map_buffer(device, vertex_buffer, vertex_data, VerticesSize);
    for (size_t i = 0; i < VerticesCount; i++) {
        printf("Vertex #%zu: x=%f, y=%f\n", i, vertex_data[i].position[0], vertex_data[i].position[1]);
    }
    free(vertex_data);

    uint16_t *indices_data = malloc(IndicesSize);
    map_buffer(device, index_buffer, indices_data, IndicesSize);
    for (size_t i = 0; i < IndicesCount; i++) {
        printf("Index #%zu: %d\n", i, indices_data[i]);
    }
    free(indices_data);

    // finish loading data

    Camera camera = {0};
    camera_init(&camera);
    bool running = true;
    SDL_Event e;
    uint64_t now, last_frame_time = SDL_GetTicks();

    while (running) {
        while (SDL_PollEvent(&e)) {
            switch (e.type) {
            case SDL_EVENT_KEY_DOWN: {
                switch (e.key.scancode) {
                case SDL_SCANCODE_ESCAPE: {
                    running = false;
                } break;
                case SDL_SCANCODE_UP: {
                    vec3 dest = {0};
                    glm_vec3_sub(camera.position, camera.target, dest);
                    glm_vec3_divs(dest, 10, dest);
                    glm_vec3_add(camera.position, dest, camera.position);
                    camera_set_view(&camera);
                } break;
                case SDL_SCANCODE_DOWN: {
                    vec3 dest = {0};
                    glm_vec3_sub(camera.position, camera.target, dest);
                    glm_vec3_divs(dest, 10, dest);
                    glm_vec3_sub(camera.position, dest, camera.position);
                    camera_set_view(&camera);
                } break;
                case SDL_SCANCODE_A: {
                    camera_rotate_around_point(&camera, camera.target, CAMERA_DIRECTION_LEFT, 5);
                } break;
                case SDL_SCANCODE_D: {
                    camera_rotate_around_point(&camera, camera.target, CAMERA_DIRECTION_RIGHT, 5);
                } break;
                case SDL_SCANCODE_Q: {
                    camera_strafe(&camera, CAMERA_DIRECTION_LEFT, 5);
                } break;
                case SDL_SCANCODE_E: {
                    camera_strafe(&camera, CAMERA_DIRECTION_RIGHT, 5);
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
            SDL_BindGPUGraphicsPipeline(render_pass, pipeline);
            SDL_BindGPUVertexBuffers(render_pass, 0, &(SDL_GPUBufferBinding){.buffer = vertex_buffer, .offset = 0}, 1);
            SDL_BindGPUIndexBuffer(render_pass, &(SDL_GPUBufferBinding){.buffer = index_buffer, .offset = 0},
                                   SDL_GPU_INDEXELEMENTSIZE_16BIT);
            SDL_DrawGPUIndexedPrimitives(render_pass, IndicesCount, IndicesCount, 0, 0, 0);
            SDL_EndGPURenderPass(render_pass);

            CHECK(SDL_SubmitGPUCommandBuffer(cmdbuf));
        }
    }
    return 0;
}
