#include "SDL3/SDL_gpu.h"
#include "SDL3/SDL_scancode.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <cglm/cglm.h>

#include <cimgui.h>
#include <cimgui_impl.h>

#include <SDL3/SDL.h>
#include <SDL3/SDL_video.h>

#include "camera.h"
#include "constants.h"
#include "pipeline.h"

int main() {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        fprintf(stderr, "Failed to init video! %s", SDL_GetError());
        return 1;
    };

    SDL_Window *window = NULL;
    window = SDL_CreateWindow("Dung", SCREEN_WIDTH, SCREEN_HEIGHT, 0);
    assert(window);

    SDL_GPUDevice *device = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_MSL, true, NULL);
    assert(device);

    const char *device_driver = SDL_GetGPUDeviceDriver(device);
    CHECK(device_driver);

    CHECK(SDL_ClaimWindowForGPUDevice(device, window));

    // setup imgui
    igCreateContext(NULL);

    // set docking
    ImGuiIO *ioptr = igGetIO_Nil();
    ioptr->ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
    // ioptr->ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
#ifdef IMGUI_HAS_DOCK
    ioptr->ConfigFlags |= ImGuiConfigFlags_DockingEnable;   // Enable Docking
    ioptr->ConfigFlags |= ImGuiConfigFlags_ViewportsEnable; // Enable Multi-Viewport / Platform Windows
#endif

    assert(ImGui_ImplSDL3_InitForSDLGPU(window));
    assert(ImGui_ImplSDLGPU3_Init(&(ImGui_ImplSDLGPU3_InitInfo){
        .ColorTargetFormat = SDL_GetGPUSwapchainTextureFormat(device, window),
        .Device = device,
        .MSAASamples = SDL_GPU_SAMPLECOUNT_1,
    }));

    Pipeline cube_pipeline;
    cube_pipeline_init(&cube_pipeline, window, device);

    Pipeline floor_tile_pipeline;
    floor_tile_pipeline_init(&floor_tile_pipeline, window, device);

    // finish loading data

    Camera camera = {0};
    camera_init(&camera);
    bool running = true;
    SDL_Event e;
    uint64_t now, previous, last_frame_time = SDL_GetTicks();
    const bool *keyboard_state = SDL_GetKeyboardState(NULL);
    bool demo_window_open = true;
    bool show_cube = true;
    bool show_tiles = true;

    while (running) {

        uint64_t ts = now - previous;

        if (keyboard_state[SDL_SCANCODE_D])
            camera_rotate_around_point(&camera, camera.target, CAMERA_DIRECTION_RIGHT, ts);

        if (keyboard_state[SDL_SCANCODE_A])
            camera_rotate_around_point(&camera, camera.target, CAMERA_DIRECTION_LEFT, ts);

        if (keyboard_state[SDL_SCANCODE_S] || keyboard_state[SDL_SCANCODE_DOWN])
            camera_zoom(&camera, CAMERA_ZOOM_IN, ts);

        if (keyboard_state[SDL_SCANCODE_W] || keyboard_state[SDL_SCANCODE_UP])
            camera_zoom(&camera, CAMERA_ZOOM_OUT, ts);

        if (keyboard_state[SDL_SCANCODE_Q])
            camera_strafe(&camera, CAMERA_DIRECTION_LEFT, ts);

        if (keyboard_state[SDL_SCANCODE_E])
            camera_strafe(&camera, CAMERA_DIRECTION_RIGHT, ts);

        while (SDL_PollEvent(&e)) {
            ImGui_ImplSDL3_ProcessEvent(&e);
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

        previous = now;
        now = SDL_GetTicks();
        if (now - last_frame_time >= SCREEN_TICKS_PER_FRAME) {
            last_frame_time = now;

            ImGui_ImplSDLGPU3_NewFrame();
            ImGui_ImplSDL3_NewFrame();
            igNewFrame();

            {
                igBegin("Debug", &demo_window_open, 0);
                igCheckbox("Show Cube", &show_cube);
                igCheckbox("Show Tiles", &show_tiles);
                igEnd();
            }

            igRender();

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

            ImDrawData *imgui_draw_data = igGetDrawData();
            Imgui_ImplSDLGPU3_PrepareDrawData(imgui_draw_data, cmdbuf);

            SDL_GPURenderPass *render_pass = SDL_BeginGPURenderPass(cmdbuf, &color_target_info, 1, NULL);
            CHECK(render_pass);

            SDL_PushGPUVertexUniformData(cmdbuf, 0, camera.mvp, sizeof(mat4));

            if (show_cube) {
                pipeline_render(&cube_pipeline, render_pass);
            }
            if (show_tiles) {
                pipeline_render(&floor_tile_pipeline, render_pass);
            }

            ImGui_ImplSDLGPU3_RenderDrawData(imgui_draw_data, cmdbuf, render_pass, NULL);

            SDL_EndGPURenderPass(render_pass);
            CHECK(SDL_SubmitGPUCommandBuffer(cmdbuf));
        }
    }
    return 0;
}
