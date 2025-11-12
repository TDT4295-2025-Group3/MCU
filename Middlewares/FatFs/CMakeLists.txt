# FatFs sources
set(FATFS_SRCS
    ${CMAKE_CURRENT_SOURCE_DIR}/Src/ff.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Src/ffunicode.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Src/diskio.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Src/ff_gen_drv.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Src/ffsystem_baremetal.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Src/drivers/sd_diskio.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Src/drivers/user_diskio.c
)

# FatFs include directory
set(FATFS_INC
    ${CMAKE_CURRENT_SOURCE_DIR}/Inc
    ${CMAKE_CURRENT_SOURCE_DIR}/Inc/drivers
)

# Add FatFs library target
add_library(FatFs STATIC ${FATFS_SRCS})

target_include_directories(FatFs PUBLIC ${FATFS_INC})
target_include_directories(FatFs PUBLIC
    ${CMAKE_SOURCE_DIR}/Core/Inc
    ${CMAKE_SOURCE_DIR}/Drivers/STM32U5xx_HAL_Driver/Inc
    ${CMAKE_SOURCE_DIR}/Drivers/CMSIS/Device/ST/STM32U5xx/Include
    ${CMAKE_SOURCE_DIR}/Drivers/CMSIS/Include
)
target_compile_definitions(FatFs PUBLIC
    USE_HAL_DRIVER
    STM32U545xx
)