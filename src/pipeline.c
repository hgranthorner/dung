#include "pipeline.h"
#include "constants.h"
#include "sdl_utils.h"
#include <assert.h>
#include <math.h>
#include <stdint.h>

void cube_pipeline_init(Pipeline *pipeline, SDL_Window *window, SDL_GPUDevice *device) {

    static VectorInput CubeVertices[] = {
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
    const size_t VerticesCount = sizeof(CubeVertices) / sizeof(VectorInput);
    const size_t VerticesSize = sizeof(CubeVertices);

    // clang-format off
    static uint16_t CubeIndices[] = {
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
    const size_t CubeIndicesCount = sizeof(CubeIndices) / sizeof(uint16_t);
    const size_t CubeIndicesSize = sizeof(CubeIndices);

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

    pipeline->pipeline = SDL_CreateGPUGraphicsPipeline(device, &pipeline_info);
    CHECK(pipeline->pipeline);

    SDL_ReleaseGPUShader(device, vert_shader);
    SDL_ReleaseGPUShader(device, frag_shader);

    pipeline->vertices_count = VerticesCount;
    pipeline->indices_count = CubeIndicesCount;
    pipeline->vertices = CubeVertices;
    pipeline->indices = CubeIndices;

    pipeline->vertex_buffer = SDL_CreateGPUBuffer(
        device, &(SDL_GPUBufferCreateInfo){.usage = SDL_GPU_BUFFERUSAGE_VERTEX, .size = VerticesSize});
    CHECK(pipeline->vertex_buffer);

    pipeline->index_buffer = SDL_CreateGPUBuffer(
        device, &(SDL_GPUBufferCreateInfo){.usage = SDL_GPU_BUFFERUSAGE_INDEX, .size = CubeIndicesSize});
    CHECK(pipeline->index_buffer);

    {
        SDL_GPUTransferBuffer *transfer =
            SDL_CreateGPUTransferBuffer(device, &(SDL_GPUTransferBufferCreateInfo){
                                                    .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
                                                    .size = VerticesSize + CubeIndicesSize,
                                                });
        VectorInput *vertex_data = SDL_MapGPUTransferBuffer(device, transfer, false);
        memcpy(vertex_data, CubeVertices, VerticesSize);
        uint16_t *index_data = (uint16_t *)&vertex_data[VerticesCount];
        memcpy(index_data, CubeIndices, CubeIndicesSize);

        SDL_UnmapGPUTransferBuffer(device, transfer);

        SDL_GPUCommandBuffer *upload_cmdbuf = SDL_AcquireGPUCommandBuffer(device);
        SDL_GPUCopyPass *copy_pass = SDL_BeginGPUCopyPass(upload_cmdbuf);

        SDL_UploadToGPUBuffer(
            copy_pass, &(SDL_GPUTransferBufferLocation){.transfer_buffer = transfer, .offset = 0},
            &(SDL_GPUBufferRegion){.buffer = pipeline->vertex_buffer, .offset = 0, .size = VerticesSize}, false);

        SDL_UploadToGPUBuffer(copy_pass,
                              &(SDL_GPUTransferBufferLocation){
                                  .transfer_buffer = transfer,
                                  .offset = VerticesSize,
                              },
                              &(SDL_GPUBufferRegion){
                                  .buffer = pipeline->index_buffer,
                                  .offset = 0,
                                  .size = CubeIndicesSize,
                              },
                              false);

        SDL_EndGPUCopyPass(copy_pass);
        SDL_GPUFence *fence = SDL_SubmitGPUCommandBufferAndAcquireFence(upload_cmdbuf);
        SDL_ReleaseGPUTransferBuffer(device, transfer);

        while (!SDL_QueryGPUFence(device, fence))
            ;

        SDL_ReleaseGPUFence(device, fence);
    }
}

void floor_tile_pipeline_init(Pipeline *pipeline, SDL_Window *window, SDL_GPUDevice *device) {
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
                .fill_mode = SDL_GPU_FILLMODE_LINE,
            },
    };

    pipeline->pipeline = SDL_CreateGPUGraphicsPipeline(device, &pipeline_info);
    CHECK(pipeline->pipeline);

    SDL_ReleaseGPUShader(device, vert_shader);
    SDL_ReleaseGPUShader(device, frag_shader);

    pipeline->vertices_count = 100;
    const size_t TileVerticesSize = sizeof(VectorInput) * pipeline->vertices_count;

    pipeline->indices_count = pipeline->vertices_count * 8;
    const size_t TileIndicesSize = sizeof(uint16_t) * pipeline->indices_count;

    pipeline->vertex_buffer = SDL_CreateGPUBuffer(
        device, &(SDL_GPUBufferCreateInfo){.usage = SDL_GPU_BUFFERUSAGE_VERTEX, .size = TileVerticesSize});
    CHECK(pipeline->vertex_buffer);

    pipeline->index_buffer = SDL_CreateGPUBuffer(
        device, &(SDL_GPUBufferCreateInfo){.usage = SDL_GPU_BUFFERUSAGE_INDEX, .size = TileIndicesSize});
    CHECK(pipeline->index_buffer);

    {
        SDL_GPUTransferBuffer *transfer =
            SDL_CreateGPUTransferBuffer(device, &(SDL_GPUTransferBufferCreateInfo){
                                                    .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
                                                    .size = TileVerticesSize + TileIndicesSize,
                                                });
        vec4 white = {COLOR_WHITE.r, COLOR_WHITE.g, COLOR_WHITE.b, COLOR_WHITE.a};
        VectorInput *vertex_data = SDL_MapGPUTransferBuffer(device, transfer, false);
        size_t rows, cols = sqrt(pipeline->vertices_count);
        assert(rows * cols == pipeline->vertices_count);
        size_t start = -100;
        size_t end = 100;
        vec4 pos = {0};
        pos[0] = -((float)start / 2);
        /* 0123456789
       -50 ..........
       -40 ..........
       -30 ..........
       -20 ..........
       -10 ..........
         0 ..........
        10 ..........
        20 ..........
        30 ..........
        40 ..........
            */
        for (size_t row = 0; row < rows; row++) {
            pos[2] = -((float)start / 2);
            for (size_t col = 0; col < cols; col++) {
                size_t i = row * rows + col;
                glm_vec4_copy(pos, vertex_data[i].position);
                glm_vec4_copy(white, vertex_data[i].color);
                pos[2] += 10;
            }
            pos[0] += 10;
        }

        uint16_t *index_data = (uint16_t *)&vertex_data[pipeline->vertices_count];
        // TODO - figure this out
        bool doing_top = true;
        size_t i = 0;
        for (size_t row = 0; row < rows - 1; row++) {
            if (doing_top) {
                /*
                 * 0123456789
                 * 0123456789
                 *
                 * top - (col, col + 1, col + 1 + 10)
                 * 01
                 *  2
                 * bot - (col + 1, col + 10, col + 9)
                 * 2
                 * 10
                 */
                for (size_t col = 0; col < cols - 1; col += 2) {
                    index_data[i++] = row * row + col;
                    index_data[i++] = row * row + col + 1;
                    index_data[i++] = row * (row + 1) + col;
                }
            } else {
                for (size_t col = 0; col < cols; col++) {
                    size_t i = row * rows + col;
                    glm_vec4_copy(pos, vertex_data[i].position);
                    glm_vec4_copy(white, vertex_data[i].color);
                    pos[2] += 10;
                }
            }
            if (!doing_top)
                row--;
            doing_top = !doing_top;
        }

        SDL_UnmapGPUTransferBuffer(device, transfer);

        SDL_GPUCommandBuffer *upload_cmdbuf = SDL_AcquireGPUCommandBuffer(device);
        SDL_GPUCopyPass *copy_pass = SDL_BeginGPUCopyPass(upload_cmdbuf);

        SDL_UploadToGPUBuffer(
            copy_pass, &(SDL_GPUTransferBufferLocation){.transfer_buffer = transfer, .offset = 0},
            &(SDL_GPUBufferRegion){.buffer = pipeline->vertex_buffer, .offset = 0, .size = TileVerticesSize}, false);

        SDL_UploadToGPUBuffer(copy_pass,
                              &(SDL_GPUTransferBufferLocation){
                                  .transfer_buffer = transfer,
                                  .offset = TileVerticesSize,
                              },
                              &(SDL_GPUBufferRegion){
                                  .buffer = pipeline->index_buffer,
                                  .offset = 0,
                                  .size = TileIndicesSize,
                              },
                              false);

        SDL_EndGPUCopyPass(copy_pass);
        SDL_GPUFence *fence = SDL_SubmitGPUCommandBufferAndAcquireFence(upload_cmdbuf);
        SDL_ReleaseGPUTransferBuffer(device, transfer);

        while (!SDL_QueryGPUFence(device, fence))
            ;

        SDL_ReleaseGPUFence(device, fence);
    }
}
