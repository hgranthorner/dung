cmake_minimum_required(VERSION 3.30)
project(dung C)

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

FetchContent_MakeAvailable(sdl3 cglm)

add_executable(${PROJECT_NAME} 
	src/main.c
	src/sdl_utils.c
	src/constants.c
	src/camera.c
	src/pipeline.c
)
target_link_libraries(${PROJECT_NAME} PRIVATE SDL3::SDL3 cglm)
