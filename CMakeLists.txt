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
	EXCLUDE_FROM_ALL
	GIT_REPOSITORY https://github.com/cimgui/cimgui.git 
	GIT_HASH 140d107
	GIT_SUBMODULES_RECURSE true
	GIT_PROGRESS true
	GIT_SHALLOW true
)

FetchContent_MakeAvailable(cimgui sdl3 cglm)

file(READ ${cimgui_SOURCE_DIR}/generator/output/cimgui_impl.h cimgui_impl)
string(FIND "${cimgui_impl}" "SDLGPU3" sdlgpu_position)
message("Found: ${sdlgpu_position}")
if(sdlgpu_position EQUAL -1)
	execute_process(
		COMMAND luajit generator.lua clang internal sdl3 sdlgpu3 -I../imgui -I${sdl3_SOURCE_DIR}/include
		WORKING_DIRECTORY ${cimgui_SOURCE_DIR}/generator
	)
endif()

add_library(cimgui_with_backends SHARED 
	${cimgui_SOURCE_DIR}/cimgui.cpp
	${cimgui_SOURCE_DIR}/imgui/imgui.cpp
	${cimgui_SOURCE_DIR}/imgui/imgui_draw.cpp
	${cimgui_SOURCE_DIR}/imgui/imgui_demo.cpp
	${cimgui_SOURCE_DIR}/imgui/imgui_widgets.cpp
	${cimgui_SOURCE_DIR}/imgui/imgui_tables.cpp
	${cimgui_SOURCE_DIR}/imgui/backends/imgui_impl_sdl3.cpp
	${cimgui_SOURCE_DIR}/imgui/backends/imgui_impl_sdlgpu3.cpp
)

target_include_directories(cimgui_with_backends PUBLIC ${cimgui_SOURCE_DIR} ${cimgui_SOURCE_DIR}/generator/output)
target_include_directories(cimgui_with_backends PRIVATE ${cimgui_SOURCE_DIR}/imgui ${cimgui_SOURCE_DIR}/imgui/backends)
target_compile_definitions(cimgui_with_backends PRIVATE "-DIMGUI_IMPL_API=extern\t\"C\"\t")
target_compile_features(cimgui_with_backends PRIVATE cxx_std_11)
target_link_libraries(cimgui_with_backends PRIVATE SDL3::SDL3)

add_executable(${PROJECT_NAME} 
	src/main.c
	src/sdl_utils.c
	src/constants.c
	src/camera.c
	src/pipeline.c
)

target_include_directories(${PROJECT_NAME} PRIVATE ${cimgui_SOURCE_DIR}/generator/output)
target_link_libraries(${PROJECT_NAME} PRIVATE SDL3::SDL3 cglm cimgui_with_backends)
target_compile_definitions(
	${PROJECT_NAME}
	PRIVATE 
	CIMGUI_DEFINE_ENUMS_AND_STRUCTS=1
	CIMGUI_USE_SDL3=1
	CIMGUI_USE_SDLGPU3=1
)
