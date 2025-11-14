cmake_minimum_required(VERSION 3.22)

# -------------------------
# Project & languages
# -------------------------
project(STMU595VJT6 LANGUAGES C CXX ASM)
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

# Build switches
option(BUILD_STM32 "Build the STM32 firmware target" ON)
option(BUILD_SIMULATOR "Build the PC simulator target" ON)
option(BUILD_SPI_TESTS "Build SPI protocol unit tests" OFF)

# -------------------------
# Compiler standards
# -------------------------
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)

# Define the build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()
# Set the project name
set(CMAKE_PROJECT_NAME STMU595VJT6)

# Export compile_commands.json for clangd, etc.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Core project settings
project(${CMAKE_PROJECT_NAME})
message("Build type: " ${CMAKE_BUILD_TYPE})

# Enable CMake support for ASM and C languages
enable_language(C CXX ASM)

# -------------------------
# Shared sources/includes
# -------------------------
set(APP_SRC
        Core/Src/app/game.cpp
        Core/Src/app/game.hpp
        Core/Src/app/constants.hpp
        Core/Src/app/math.hpp
        Core/Src/app/player.hpp
        Core/Src/app/player.cpp
        Core/Src/app/camera.hpp
        Core/Src/app/camera.cpp
        Core/Src/app/input.hpp
        Core/Src/app/model_loader.hpp
        Core/Src/app/model_loader.cpp
        Core/Src/device/spi_stm.hpp
        Core/Src/hid_input/hid_driver.hpp
        Core/Src/hid_input/hid_reports.h
        Core/Src/hid_input/hid_driver_ds4.cpp
        Core/Src/hid_input/hid_driver_kb.cpp
)

set(APP_INC
        ${CMAKE_SOURCE_DIR}/Core/Src/app
        ${CMAKE_SOURCE_DIR}/Core/Src/app/platform
        ${CMAKE_SOURCE_DIR}/Core/Src/hid_input
)

# -------------------------
# Unity (Unit Test Lib)
# -------------------------
add_library(Unity STATIC
    Core/External/Unity/src/unity.c
)

target_include_directories(Unity PUBLIC
    Core/External/Unity/src
)

# -------------------------
# STM32 firmware (cross)
# -------------------------
# --- STM32 firmware (cross) ---

if (BUILD_STM32)

    # --- ADD FatFs middleware build ---
    add_subdirectory(Middlewares/FatFs)

    set(TINYUSB_DIR ${CMAKE_SOURCE_DIR}/tinyusb/src)

    set(TINYUSB_INC
            ${TINYUSB_DIR}
            ${TINYUSB_DIR}/common
            ${TINYUSB_DIR}/class/hid
            ${TINYUSB_DIR}/host
            ${TINYUSB_DIR}/portable/synopsys/dwc2
    )

    set(TINYUSB_SRC
            ${TINYUSB_DIR}/tusb.c
            ${TINYUSB_DIR}/common/tusb_fifo.c
            ${TINYUSB_DIR}/class/hid/hid_host.c
            ${TINYUSB_DIR}/host/usbh.c
            ${TINYUSB_DIR}/host/hub.c
            ${TINYUSB_DIR}/portable/synopsys/dwc2/hcd_dwc2.c
            ${TINYUSB_DIR}/portable/synopsys/dwc2/dwc2_common.c
    )

    add_executable(${PROJECT_NAME}
            ${APP_SRC}
            ${TINYUSB_SRC}
            Core/Src/device/spi_stm.cpp
            Core/Src/device/spi_stm.hpp
            Core/Src/device/seven_seg_display.cpp
            Core/Src/device/seven_seg_display_c.cpp
            Core/Src/device/hid_input/tusb_config.h
            Core/Src/device/hid_input/tusb_input.cpp
            Core/Src/device/hid_input/tusb_input.hpp

    )

    add_subdirectory(cmake/stm32cubemx)

    target_include_directories(${PROJECT_NAME} PRIVATE
            ${APP_INC}
            ${CMAKE_SOURCE_DIR}/Core/Src/device
            ${CMAKE_SOURCE_DIR}/Core/Src/device/hid_input
            ${CMAKE_SOURCE_DIR}/Core/External/Unity/src
            ${CMAKE_SOURCE_DIR}/Middlewares/FatFs/Inc
            ${CMAKE_SOURCE_DIR}/Middlewares/FatFs/Inc/drivers
            ${TINYUSB_INC}
    )

    target_compile_definitions(${PROJECT_NAME} PRIVATE PLATFORM_STM32=1)

    # If SPI tests are enabled, compile firmware with SPI test mode so main() can call spi_test_main()
    if (BUILD_SPI_TESTS)
        target_compile_definitions(${PROJECT_NAME} PRIVATE SPI_TEST_MODE=1)
    endif()

    # If SPI tests are enabled, also compile the test harness into the firmware so
    # spi_test_main() is available from main(). This keeps a single firmware ELF
    # that runs tests at startup.
    if (BUILD_SPI_TESTS)
        target_sources(${PROJECT_NAME} PRIVATE
            Core/Test/SPITests/spiTestHarness.c
            Core/Test/SPITests/ProtocolTests.cpp
        )
        target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/Core/Test/SPITests)
        # Unity is already linked below; ensure it's available for the test harness
    endif()

    # keep your libob quirk
    list(REMOVE_ITEM CMAKE_C_IMPLICIT_LINK_LIBRARIES ob)

    # ⬇⬇⬇ Change to the PLAIN signature (no PRIVATE/PUBLIC/INTERFACE)
    target_link_libraries(${PROJECT_NAME}
            stm32cubemx
            
            # add other libs here, still plain
            Unity
            FatFs
    )
endif()

# -------------------------
# STM32 SPI/Integration tests
# -------------------------
if (BUILD_STM32 AND BUILD_SPI_TESTS)
    # Build an ELF that runs the Unity-based SPI protocol tests on the MCU.
    add_executable(test
        ${APP_SRC}
        Core/Test/SPITests/spiTestHarness.c
        Core/Test/SPITests/ProtocolTests.cpp
        Core/Src/device/spi_stm.cpp
        Core/Src/device/seven_seg_display.cpp
        Core/Src/device/seven_seg_display_c.cpp
        Core/Src/device/spi_stm.hpp
        Core/Src/main.cpp
        Core/Src/sysmem.c
        Core/Src/syscalls.c
        startup_stm32u595xx.s
    )

    # Include same directories as firmware so tests can access HAL headers and project headers
    target_include_directories(test PRIVATE
        ${APP_INC}
        ${CMAKE_SOURCE_DIR}/Core/Src/device
        ${CMAKE_SOURCE_DIR}/Core/Test/SPITests
        ${CMAKE_SOURCE_DIR}/Core/Src
        ${CMAKE_SOURCE_DIR}/Core/Src/app
        ${CMAKE_SOURCE_DIR}/Core/Src/Device
        ${CMAKE_SOURCE_DIR}/Core/Src
        ${CMAKE_SOURCE_DIR}/Core/External/Unity/src
        ${CMAKE_SOURCE_DIR}/Middlewares/FatFs/Inc
        ${CMAKE_SOURCE_DIR}/Middlewares/FatFs/Inc/drivers
    )

    target_compile_definitions(test PRIVATE PLATFORM_STM32=1)

    # Link with the STM32 support and Unity test lib
    # stm32cubemx is provided by the cmake/stm32cubemx subdirectory
    # Link with STM32CubeMX generated drivers (OBJECT library) and Unity
    # STM32_Drivers is created in cmake/stm32cubemx/CMakeLists.txt as an OBJECT library
    target_link_libraries(test
        stm32cubemx
        STM32_Drivers
        Unity
        FatFs
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
            Core/Src/sim/host_input_timer.cpp
            Core/Src/sim/host_rasterizer.cpp
            Core/Src/sim/small3dlib.h
    )

    target_include_directories(simulator PRIVATE
            ${APP_INC}
            ${CMAKE_SOURCE_DIR}/Core/Src/sim
            ${CMAKE_SOURCE_DIR}/Core/Src/device
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

    find_package(hidapi REQUIRED)
    target_link_libraries(simulator PRIVATE hidapi::hidapi)

    target_compile_definitions(simulator PRIVATE PLATFORM_PC=1)
endif()