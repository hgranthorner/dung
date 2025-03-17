cmake_minimum_required(VERSION 3.30)
project(dung LANGUAGES C CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS true)
set(CMAKE_C_STANDARD 11)

include(FetchContent)

FetchContent_Declare(
	sdl3
	URL https://github.com/libsdl-org/SDL/releases/download/release-3.2.8/SDL3-3.2.8.tar.gz
)

FetchContent_Declare(
	cglm
	URL https://github.com/recp/cglm/archive/refs/tags/v0.9.6.tar.gz
)

FetchContent_Declare(
	cimgui
	GIT_REPOSITORY https://github.com/hgranthorner/cimgui.git 
	GIT_TAG origin/add_sdlgpu3_backend
	GIT_SUBMODULES_RECURSE true
	GIT_PROGRESS true
	GIT_SHALLOW true
)

FetchContent_MakeAvailable(cimgui sdl3 cglm)

add_library(
	cimgui_sdlgpu3_backend 
	SHARED 
	${cimgui_SOURCE_DIR}/imgui/backends/imgui_impl_sdl3.cpp
	${cimgui_SOURCE_DIR}/imgui/backends/imgui_impl_sdlgpu3.cpp
)
target_include_directories(cimgui_sdlgpu3_backend PRIVATE ${cimgui_SOURCE_DIR}/imgui)
target_compile_features(cimgui_sdlgpu3_backend PRIVATE cxx_std_11)
target_link_libraries(cimgui_sdlgpu3_backend PRIVATE SDL3::SDL3 cimgui)
target_compile_definitions(cimgui_sdlgpu3_backend PUBLIC IMGUI_IMPL_API=extern\t\"C\"\t)

add_executable(${PROJECT_NAME} 
	src/main.c
	src/sdl_utils.c
	src/constants.c
	src/camera.c
	src/pipeline.c
)

target_include_directories(${PROJECT_NAME} PRIVATE ${cimgui_SOURCE_DIR}/generator/output)
target_link_libraries(${PROJECT_NAME} PRIVATE SDL3::SDL3 cglm cimgui cimgui_sdlgpu3_backend)
target_compile_definitions(
	${PROJECT_NAME}
	PRIVATE 
	CIMGUI_DEFINE_ENUMS_AND_STRUCTS=1
	CIMGUI_USE_SDL3=1
	CIMGUI_USE_SDLGPU3=1
)
