#include "SDL3/SDL_gpu.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <SDL3/SDL.h>
#include <SDL3/SDL_video.h>

typedef struct {
  float x, y, z;
} Vector3f;

typedef struct {
  float x, y, z, w;
} Vector4f;

const int SCREEN_WIDTH = 640;
const int SCREEN_HEIGHT = 480;
const int SCREEN_FPS = 60;
const int SCREEN_TICKS_PER_FRAME = 1000 / SCREEN_FPS;

const SDL_FColor COLOR_WHITE = (SDL_FColor){1.0f, 1.0f, 1.0f, 1.0f};
const SDL_FColor COLOR_BLACK = (SDL_FColor){0.0f, 0.0f, 0.0f, 1.0f};
const SDL_FColor COLOR_RED = (SDL_FColor){1.0f, 0.0f, 0.0f, 1.0f};
const SDL_FColor COLOR_GREEN = (SDL_FColor){0.0f, 1.0f, 0.0f, 1.0f};
const SDL_FColor COLOR_BLUE = (SDL_FColor){0.0f, 0.0f, 1.0f, 1.0f};
const SDL_FColor COLOR_CYAN = (SDL_FColor){0.0f, 1.0f, 1.0f, 1.0f};
const SDL_FColor COLOR_YELLOW = (SDL_FColor){1.0f, 1.0f, 0.0f, 1.0f};
const SDL_FColor COLOR_PINK = (SDL_FColor){1.0f, 0.0f, 1.0f, 1.0f};

#define NUM_VERTICES 4
const Vector4f VERTICES[NUM_VERTICES] = {
    {.x = -0.5, .y = -0.5, .z = 0, .w = 1},
    {.x = -0.5, .y = 0.5, .z = 0, .w = 1},
    {.x = 0.5, .y = -0.5, .z = 0, .w = 1},
    {.x = 0.5, .y = -0.5, .z = 0, .w = 1},
};
const size_t VERTICES_SIZE = sizeof(Vector4f) * NUM_VERTICES;

#define NUM_INDICES 6
const uint32_t INDICES[NUM_INDICES] = {
    0, 1, 2, //
    1, 4, 2  //
};
const size_t INDICES_SIZE = sizeof(INDICES) * NUM_INDICES;

#define CHECK(x)                                                               \
  do {                                                                         \
    if (!x) {                                                                  \
      fprintf(stderr, "SDL Error: %s", SDL_GetError());                        \
      exit(1);                                                                 \
    }                                                                          \
  } while (0)

void map_buffer(SDL_GPUDevice *device, SDL_GPUBuffer *buffer, void *dest,
                size_t size) {
  SDL_GPUTransferBuffer *transfer_buffer = SDL_CreateGPUTransferBuffer(
      device, &(SDL_GPUTransferBufferCreateInfo){
                  .usage = SDL_GPU_TRANSFERBUFFERUSAGE_DOWNLOAD,
                  .size = size,
              });

  SDL_GPUCommandBuffer *download_cmdbuf = SDL_AcquireGPUCommandBuffer(device);
  SDL_GPUCopyPass *copy_pass = SDL_BeginGPUCopyPass(download_cmdbuf);

  SDL_GPUTransferBufferLocation loc = {.transfer_buffer = transfer_buffer,
                                       .offset = 0};
  SDL_GPUBufferRegion reg = {.buffer = buffer, .offset = 0, .size = size};
  SDL_DownloadFromGPUBuffer(copy_pass, &reg, &loc);
  SDL_EndGPUCopyPass(copy_pass);

  SDL_GPUFence *fence =
      SDL_SubmitGPUCommandBufferAndAcquireFence(download_cmdbuf);
  CHECK(fence);

  while (!SDL_QueryGPUFence(device, fence)) {
  };

  SDL_ReleaseGPUFence(device, fence);

  Vector4f *transfer_data =
      SDL_MapGPUTransferBuffer(device, transfer_buffer, false);

  memcpy(dest, transfer_data, size);

  SDL_UnmapGPUTransferBuffer(device, transfer_buffer);
  SDL_ReleaseGPUTransferBuffer(device, transfer_buffer);
}

SDL_GPUShader *load_shader(SDL_GPUDevice *device, const char *filename,
                           SDL_GPUShaderStage stage, Uint32 sampler_count,
                           Uint32 uniform_buffer_count,
                           Uint32 storage_buffer_count,
                           Uint32 storage_texture_count) {

  if (!SDL_GetPathInfo(filename, NULL)) {
    fprintf(stdout, "File (%s) does not exist.\n", filename);
    return NULL;
  }

  const char *entrypoint;
  SDL_GPUShaderFormat backend_formats = SDL_GetGPUShaderFormats(device);
  SDL_GPUShaderFormat format = SDL_GPU_SHADERFORMAT_MSL;
  if (stage == SDL_GPU_SHADERSTAGE_FRAGMENT) {
    entrypoint = "fragmentShader";
  } else if (stage == SDL_GPU_SHADERSTAGE_VERTEX) {
    entrypoint = "vertexShader";
  }

  size_t code_size;
  void *code = SDL_LoadFile(filename, &code_size);
  if (code == NULL) {
    fprintf(stderr, "ERROR: SDL_LoadFile(%s) failed: %s\n", filename,
            SDL_GetError());
    return NULL;
  }

  SDL_GPUShaderCreateInfo shader_info = {
      .code = code,
      .code_size = code_size,
      .entrypoint = entrypoint,
      .format = format,
      .stage = stage,
      .num_samplers = sampler_count,
      .num_uniform_buffers = uniform_buffer_count,
      .num_storage_buffers = storage_buffer_count,
      .num_storage_textures = storage_texture_count,
  };

  SDL_GPUShader *shader = SDL_CreateGPUShader(device, &shader_info);

  if (shader == NULL) {
    fprintf(stderr, "ERROR: SDL_CreateGPUShader failed: %s\n", SDL_GetError());
    SDL_free(code);
    return NULL;
  }
  SDL_free(code);
  return shader;
}

int main() {

  if (!SDL_Init(SDL_INIT_VIDEO)) {
    fprintf(stderr, "Failed to init video! %s", SDL_GetError());
    return 1;
  };

  SDL_Window *window = NULL;
  // SDL_Renderer *renderer = NULL;
  if (!(window = SDL_CreateWindow("Dung", SCREEN_WIDTH, SCREEN_HEIGHT, 0))) {
    // if (!SDL_CreateWindowAndRenderer("Dung", SCREEN_WIDTH, SCREEN_HEIGHT, 0,
    //                                  &window, &renderer)) {
    fprintf(stderr, "Failed to create window! %s", SDL_GetError());
    return 1;
  }

  SDL_GPUDevice *device =
      SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_MSL, true, NULL);
  CHECK(device);

  const char *device_driver = SDL_GetGPUDeviceDriver(device);
  CHECK(device_driver);
  SDL_Log("Device driver: %s", device_driver);

  CHECK(SDL_ClaimWindowForGPUDevice(device, window));

  SDL_GPUShader *vert_shader = load_shader(
      device, "src/vert.metal", SDL_GPU_SHADERSTAGE_VERTEX, 0, 0, 0, 0);
  CHECK(vert_shader);

  SDL_GPUShader *frag_shader = load_shader(
      device, "src/frag.metal", SDL_GPU_SHADERSTAGE_FRAGMENT, 0, 0, 0, 0);
  CHECK(frag_shader);

  SDL_GPUGraphicsPipelineCreateInfo pipeline_info = {
      .target_info =
          {
              .num_color_targets = 1,
              .color_target_descriptions =
                  (SDL_GPUColorTargetDescription[]){
                      {.format =
                           SDL_GetGPUSwapchainTextureFormat(device, window)}},
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
                          .pitch = sizeof(Vector4f),
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
                  },
              .num_vertex_attributes = 1,
          },
      .rasterizer_state =
          (SDL_GPURasterizerState){
              .cull_mode = SDL_GPU_CULLMODE_NONE,
              .fill_mode = SDL_GPU_FILLMODE_FILL,
          },
  };

  SDL_GPUGraphicsPipeline *pipeline =
      SDL_CreateGPUGraphicsPipeline(device, &pipeline_info);
  CHECK(pipeline);

  SDL_ReleaseGPUShader(device, vert_shader);
  SDL_ReleaseGPUShader(device, frag_shader);

  SDL_GPUBuffer *vertex_buffer = SDL_CreateGPUBuffer(
      device, &(SDL_GPUBufferCreateInfo){.usage = SDL_GPU_BUFFERUSAGE_VERTEX,
                                         .size = VERTICES_SIZE});
  CHECK(vertex_buffer);

  SDL_GPUBuffer *index_buffer = SDL_CreateGPUBuffer(
      device, &(SDL_GPUBufferCreateInfo){.usage = SDL_GPU_BUFFERUSAGE_INDEX,
                                         .size = INDICES_SIZE});
  CHECK(index_buffer);

  // load static triangle to gpu
  {
    SDL_GPUTransferBuffer *vertex_transfer = SDL_CreateGPUTransferBuffer(
        device, &(SDL_GPUTransferBufferCreateInfo){
                    .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
                    .size = sizeof(Vector4f) * 3,
                });
    Vector4f *vertex_data =
        SDL_MapGPUTransferBuffer(device, vertex_transfer, false);
    memcpy(vertex_data, VERTICES, VERTICES_SIZE);
    SDL_UnmapGPUTransferBuffer(device, vertex_transfer);

    SDL_GPUTransferBuffer *index_transfer = SDL_CreateGPUTransferBuffer(
        device, &(SDL_GPUTransferBufferCreateInfo){
                    .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
                    .size = INDICES_SIZE,
                });
    Vector4f *index_data =
        SDL_MapGPUTransferBuffer(device, index_transfer, false);
    memcpy(index_data, INDICES, INDICES_SIZE);
    SDL_UnmapGPUTransferBuffer(device, index_transfer);

    SDL_GPUCommandBuffer *upload_cmdbuf = SDL_AcquireGPUCommandBuffer(device);
    SDL_GPUCopyPass *copy_pass = SDL_BeginGPUCopyPass(upload_cmdbuf);

    SDL_UploadToGPUBuffer(copy_pass,
                          &(SDL_GPUTransferBufferLocation){
                              .transfer_buffer = vertex_transfer, .offset = 0},
                          &(SDL_GPUBufferRegion){.buffer = vertex_buffer,
                                                 .offset = 0,
                                                 .size = VERTICES_SIZE},
                          false);

    SDL_UploadToGPUBuffer(copy_pass,
                          &(SDL_GPUTransferBufferLocation){
                              .transfer_buffer = index_transfer, .offset = 0},
                          &(SDL_GPUBufferRegion){.buffer = index_buffer,
                                                 .offset = 0,
                                                 .size = INDICES_SIZE},
                          false);

    SDL_EndGPUCopyPass(copy_pass);
    SDL_SubmitGPUCommandBuffer(upload_cmdbuf);
    SDL_ReleaseGPUTransferBuffer(device, vertex_transfer);
    SDL_ReleaseGPUTransferBuffer(device, index_transfer);
  }

  Vector4f *vertex_data = malloc(VERTICES_SIZE);
  map_buffer(device, vertex_buffer, vertex_data, VERTICES_SIZE);
  for (size_t i = 0; i < NUM_VERTICES; i++) {
    printf("Vertex #%zu: x=%f, y=%f\n", i, vertex_data[i].x, vertex_data[i].y);
  }
  free(vertex_data);

  uint32_t *indices_data = malloc(INDICES_SIZE);
  map_buffer(device, index_buffer, indices_data, INDICES_SIZE);
  for (size_t i = 0; i < NUM_INDICES; i++) {
    printf("Index #%zu: %d\n", i, indices_data[i]);
  }
  free(indices_data);

  // finish loading data

  SDL_GPUCommandBuffer *command_buffer = SDL_AcquireGPUCommandBuffer(device);
  CHECK(command_buffer);

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
        fprintf(stderr, "ERROR: SDL_AcquireGPUCommandBuffer failed: %s\n",
                SDL_GetError());
        break;
      }

      SDL_GPUTexture *swapchain_texture;
      if (!SDL_WaitAndAcquireGPUSwapchainTexture(
              cmdbuf, window, &swapchain_texture, NULL, NULL)) {
        fprintf(stderr,
                "ERROR: SDL_WaitAndAcquireGPUSwapchainTexture failed: %s\n",
                SDL_GetError());
        break;
      }

      if (swapchain_texture == NULL) {
        fprintf(stderr, "ERROR: swapchain_texture is NULL\n");
        SDL_SubmitGPUCommandBuffer(cmdbuf);
        break;
      }

      SDL_GPUColorTargetInfo color_target_info = {0};
      color_target_info.texture = swapchain_texture;
      color_target_info.clear_color = COLOR_BLUE;
      color_target_info.load_op = SDL_GPU_LOADOP_CLEAR;
      color_target_info.store_op = SDL_GPU_STOREOP_STORE;

      SDL_GPURenderPass *render_pass =
          SDL_BeginGPURenderPass(cmdbuf, &color_target_info, 1, NULL);
      CHECK(render_pass);
      SDL_BindGPUGraphicsPipeline(render_pass, pipeline);
      SDL_BindGPUVertexBuffers(
          render_pass, 0,
          &(SDL_GPUBufferBinding){.buffer = vertex_buffer, .offset = 0}, 1);
      SDL_BindGPUIndexBuffer(
          render_pass,
          &(SDL_GPUBufferBinding){.buffer = index_buffer, .offset = 0},
          SDL_GPU_INDEXELEMENTSIZE_32BIT);
      // SDL_DrawGPUIndexedPrimitives(render_pass, 3, 7, 0, 0, 0);
      SDL_DrawGPUPrimitives(render_pass, 3, 1, 0, 0);
      SDL_EndGPURenderPass(render_pass);

      CHECK(SDL_SubmitGPUCommandBuffer(cmdbuf));
    }
  }
  return 0;
}
