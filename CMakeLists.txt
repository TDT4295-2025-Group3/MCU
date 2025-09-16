cmake_minimum_required(VERSION 3.22)

# -------------------------
# Project & languages
# -------------------------
project(idontknow LANGUAGES C CXX ASM)
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

# Build switches
option(BUILD_STM32 "Build the STM32 firmware target" ON)
option(BUILD_SIMULATOR "Build the PC simulator target" ON)

# -------------------------
# Compiler standards
# -------------------------
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)

# Default build type
if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif ()

# Export compile_commands.json for clangd, etc.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# -------------------------
# Shared sources/includes
# -------------------------
set(APP_SRC
        Core/Src/app/game.cpp
        Core/Src/app/game.hpp
)

set(APP_INC
        ${CMAKE_SOURCE_DIR}/Core/Src/app
)

# -------------------------
# STM32 firmware (cross)
# -------------------------
# --- STM32 firmware (cross) ---

if (BUILD_STM32)
    add_executable(${PROJECT_NAME}
            ${APP_SRC}
            Core/Src/device/input_stm.cpp
            Core/Src/device/timer_stm.cpp
    )

    add_subdirectory(cmake/stm32cubemx)

    target_include_directories(${PROJECT_NAME} PRIVATE
            ${APP_INC}
            ${CMAKE_SOURCE_DIR}/Core/Src/device
    )

    target_compile_definitions(${PROJECT_NAME} PRIVATE PLATFORM_STM32=1)

    # keep your libob quirk
    list(REMOVE_ITEM CMAKE_C_IMPLICIT_LINK_LIBRARIES ob)

    # ⬇⬇⬇ Change to the PLAIN signature (no PRIVATE/PUBLIC/INTERFACE)
    target_link_libraries(${PROJECT_NAME}
            stm32cubemx
            # add other libs here, still plain
    )
endif()

# -------------------------
# PC Simulator (host)
# -------------------------
# Only configure simulator when requested AND not cross-compiling.
if (BUILD_SIMULATOR AND NOT CMAKE_CROSSCOMPILING)
    add_executable(simulator
            ${APP_SRC}
            Core/Src/sim/main_sim.cpp
            Core/Src/sim/host_platform.cpp
    )

    target_include_directories(simulator PRIVATE
            ${APP_INC}
    )

    # SDL3 is a host dependency; guard it from STM32 builds
    find_package(SDL3 REQUIRED)
    # Prefer the imported target if provided by your SDL3 package
    if (TARGET SDL3::SDL3)
        target_link_libraries(simulator PRIVATE SDL3::SDL3)
    else()
        target_link_libraries(simulator PRIVATE ${SDL3_LIBRARIES})
        target_include_directories(simulator PRIVATE ${SDL3_INCLUDE_DIRS})
    endif()

    target_compile_definitions(simulator PRIVATE PLATFORM_PC=1)
endif()
