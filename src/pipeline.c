#include "pipeline.h"
#include "constants.h"
#include "sdl_utils.h"

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

void renderer_init(Pipeline *pipeline, SDL_Window *window, SDL_GPUDevice *device) {
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

    pipeline->vertex_buffer = SDL_CreateGPUBuffer(
        device, &(SDL_GPUBufferCreateInfo){.usage = SDL_GPU_BUFFERUSAGE_VERTEX, .size = VerticesSize});
    CHECK(pipeline->vertex_buffer);

    pipeline->index_buffer = SDL_CreateGPUBuffer(
        device, &(SDL_GPUBufferCreateInfo){.usage = SDL_GPU_BUFFERUSAGE_INDEX, .size = IndicesSize});
    CHECK(pipeline->index_buffer);

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
}
