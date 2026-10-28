cmake_minimum_required(VERSION 3.16)
project(air_forge_sim LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)

# ---------------------------------------------------------------------------
# LVGL（源码位于 third_party/lvgl，版本 9.3.0）
# ---------------------------------------------------------------------------
set(LV_BUILD_CONF_PATH "${CMAKE_CURRENT_SOURCE_DIR}/lv_conf.h" CACHE PATH "" FORCE)
set(CONFIG_LV_BUILD_DEMOS OFF CACHE BOOL "" FORCE)
set(CONFIG_LV_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(CONFIG_LV_USE_THORVG_INTERNAL OFF CACHE BOOL "" FORCE)
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/../third_party/lvgl"
                 "${CMAKE_CURRENT_BINARY_DIR}/lvgl")

# ---------------------------------------------------------------------------
# SDL2
#   Windows：使用 third_party 里的 MinGW 开发包
#   Linux/macOS：使用系统包（pkg-config: sdl2）
# ---------------------------------------------------------------------------
if(WIN32)
    set(SDL2_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../third_party/SDL2/x86_64-w64-mingw32")

    add_library(SDL2 SHARED IMPORTED)
    set_target_properties(SDL2 PROPERTIES
        IMPORTED_LOCATION "${SDL2_ROOT}/bin/SDL2.dll"
        IMPORTED_IMPLIB   "${SDL2_ROOT}/lib/libSDL2.dll.a"
        INTERFACE_INCLUDE_DIRECTORIES "${SDL2_ROOT}/include")
else()
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(SDL2 REQUIRED IMPORTED_TARGET sdl2)
    add_library(SDL2 ALIAS PkgConfig::SDL2)
endif()

# LVGL 的 lv_sdl_* 驱动需要 SDL 头文件和链接库
target_link_libraries(lvgl PUBLIC SDL2)

# ---------------------------------------------------------------------------
# 仿真程序：sim/main.c + 共享 UI 层 ui/
# ---------------------------------------------------------------------------
file(GLOB_RECURSE UI_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/../ui/*.c")

add_executable(air_forge_sim
    main.c
    ${UI_SOURCES})

target_include_directories(air_forge_sim PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/../ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/../ui/widgets"
    "${CMAKE_CURRENT_SOURCE_DIR}/../ui/screens")

# 由我们自己的 main() 接管入口，避免经 SDL_main
target_compile_definitions(air_forge_sim PRIVATE SDL_MAIN_HANDLED)

target_link_libraries(air_forge_sim PRIVATE lvgl SDL2)

# Windows 下把 SDL2.dll 放到可执行文件旁边（Linux 用系统 libSDL2）
if(WIN32)
    add_custom_command(TARGET air_forge_sim POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${SDL2_ROOT}/bin/SDL2.dll"
                "$<TARGET_FILE_DIR:air_forge_sim>")
endif()
