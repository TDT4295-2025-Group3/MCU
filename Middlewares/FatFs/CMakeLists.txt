# FatFs sources
set(FATFS_SRCS
    ${CMAKE_CURRENT_SOURCE_DIR}/Src/ff.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Src/ffsystem.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Src/ffunicode.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Src/diskio.c
)

# FatFs include directory
set(FATFS_INC
    ${CMAKE_CURRENT_SOURCE_DIR}/Inc
)

# Add FatFs library target
add_library(FatFs STATIC ${FATFS_SRCS})

target_include_directories(FatFs PUBLIC ${FATFS_INC})