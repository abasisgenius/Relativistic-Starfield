cmake_minimum_required(VERSION 3.20)
project(relativistic-starfield LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(EMSCRIPTEN)
    add_executable(relativistic-starfield src/main.cpp)
    target_include_directories(relativistic-starfield PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/shaders)
    target_compile_options(relativistic-starfield PRIVATE -Wall -Wextra -O2)
    target_link_options(relativistic-starfield PRIVATE
        -sUSE_GLFW=3
        -sMAX_WEBGL_VERSION=2
        -sMIN_WEBGL_VERSION=2
        -sALLOW_MEMORY_GROWTH=1
        -sEXPORTED_RUNTIME_METHODS=['ccall']
        -sEXPORTED_FUNCTIONS=['_main','_set_target_beta','_set_acceleration','_set_time_scale','_set_fov','_set_exposure','_set_point_size','_set_fog_distance','_toggle_pause','_reset_from_web','_get_beta','_get_target_beta','_get_ship_speed_c','_get_ship_distance','_get_coordinate_time','_get_proper_time','_get_forward_doppler','_get_backward_doppler','_get_forward_beaming','_get_paused']
        --preload-file ${CMAKE_CURRENT_SOURCE_DIR}/shaders@shaders
        --shell-file ${CMAKE_CURRENT_SOURCE_DIR}/web/shell.html
        -o ${CMAKE_CURRENT_BINARY_DIR}/index.html
    )
else()
    include(FetchContent)

    find_package(OpenGL REQUIRED)
    find_package(glfw3 CONFIG REQUIRED)
    find_package(glm CONFIG REQUIRED)

    FetchContent_Declare(
        imgui
        GIT_REPOSITORY https://github.com/ocornut/imgui.git
        GIT_TAG v1.92.9
        GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(imgui)

    add_library(imgui STATIC
        ${imgui_SOURCE_DIR}/imgui.cpp
        ${imgui_SOURCE_DIR}/imgui_draw.cpp
        ${imgui_SOURCE_DIR}/imgui_tables.cpp
        ${imgui_SOURCE_DIR}/imgui_widgets.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
    )
    target_include_directories(imgui PUBLIC
        ${imgui_SOURCE_DIR}
        ${imgui_SOURCE_DIR}/backends
    )
    target_link_libraries(imgui PUBLIC glfw)

    add_executable(relativistic-starfield src/main.cpp)
    target_compile_options(relativistic-starfield PRIVATE -Wall -Wextra -O2)
    target_include_directories(relativistic-starfield PRIVATE ${imgui_SOURCE_DIR})
    target_link_libraries(relativistic-starfield PRIVATE glfw OpenGL::GL glm::glm imgui)
endif()
