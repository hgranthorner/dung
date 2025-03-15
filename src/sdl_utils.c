#include "sdl_utils.h"

#include "constants.h"

void load_shaders(SDL_GPUDevice *device, const char *filename, SDL_GPUShader **dest) {

    if (!SDL_GetPathInfo(filename, NULL)) {
        fprintf(stdout, "File (%s) does not exist.\n", filename);
        exit(1);
    }

    SDL_GPUShaderFormat format = SDL_GPU_SHADERFORMAT_MSL;

    size_t code_size;
    void *code = SDL_LoadFile(filename, &code_size);
    if (code == NULL) {
        fprintf(stderr, "ERROR: SDL_LoadFile(%s) failed: %s\n", filename, SDL_GetError());
        exit(1);
    }

    SDL_GPUShaderCreateInfo vertex_info = {
        .code = code,
        .code_size = code_size,
        .entrypoint = "vertexShader",
        .format = format,
        .stage = SDL_GPU_SHADERSTAGE_VERTEX,
        .num_samplers = 0,
        .num_uniform_buffers = 1,
        .num_storage_buffers = 0,
        .num_storage_textures = 0,
    };
    SDL_GPUShader *shader = SDL_CreateGPUShader(device, &vertex_info);

    if (shader == NULL) {
        fprintf(stderr, "ERROR: SDL_CreateGPUShader failed: %s\n", SDL_GetError());
        SDL_free(code);
        exit(1);
    }
    dest[0] = shader;

    SDL_GPUShaderCreateInfo fragment_info = {
        .code = code,
        .code_size = code_size,
        .entrypoint = "fragmentShader",
        .format = format,
        .stage = SDL_GPU_SHADERSTAGE_FRAGMENT,
        .num_samplers = 0,
        .num_uniform_buffers = 0,
        .num_storage_buffers = 0,
        .num_storage_textures = 0,
    };
    shader = SDL_CreateGPUShader(device, &fragment_info);

    if (shader == NULL) {
        fprintf(stderr, "ERROR: SDL_CreateGPUShader failed: %s\n", SDL_GetError());
        SDL_free(code);
        exit(1);
    }
    dest[1] = shader;

    SDL_free(code);
}

void map_buffer(SDL_GPUDevice *device, SDL_GPUBuffer *buffer, void *dest, size_t size) {
    SDL_GPUTransferBuffer *transfer_buffer =
        SDL_CreateGPUTransferBuffer(device, &(SDL_GPUTransferBufferCreateInfo){
                                                .usage = SDL_GPU_TRANSFERBUFFERUSAGE_DOWNLOAD,
                                                .size = size,
                                            });

    SDL_GPUCommandBuffer *download_cmdbuf = SDL_AcquireGPUCommandBuffer(device);
    SDL_GPUCopyPass *copy_pass = SDL_BeginGPUCopyPass(download_cmdbuf);

    SDL_GPUTransferBufferLocation loc = {.transfer_buffer = transfer_buffer, .offset = 0};
    SDL_GPUBufferRegion reg = {.buffer = buffer, .offset = 0, .size = size};
    SDL_DownloadFromGPUBuffer(copy_pass, &reg, &loc);
    SDL_EndGPUCopyPass(copy_pass);

    SDL_GPUFence *fence = SDL_SubmitGPUCommandBufferAndAcquireFence(download_cmdbuf);
    CHECK(fence);

    while (!SDL_QueryGPUFence(device, fence))
        ;

    SDL_ReleaseGPUFence(device, fence);

    void *transfer_data = SDL_MapGPUTransferBuffer(device, transfer_buffer, false);

    memcpy(dest, transfer_data, size);

    SDL_UnmapGPUTransferBuffer(device, transfer_buffer);
    SDL_ReleaseGPUTransferBuffer(device, transfer_buffer);
}
