cmake_minimum_required(VERSION 3.10)
project(mgsplayer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find SDL2
if(WIN32)
    if(NOT SDL2_PATH)
        set(SDL2_PATH "${CMAKE_SOURCE_DIR}/SDL2-2.30.0/x86_64-w64-mingw32")
    endif()
    include_directories(${SDL2_PATH}/include)
    set(SDL2_LIBRARIES "${SDL2_PATH}/lib/libSDL2.a")
    set(SDL2_MAIN_LIBRARY "${SDL2_PATH}/lib/libSDL2main.a")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static -static-libgcc -static-libstdc++")
else()
    find_package(SDL2 REQUIRED)
    include_directories(${SDL2_INCLUDE_DIRS})
endif()

# Include libkss headers
include_directories(
    ${CMAKE_SOURCE_DIR}/libkss/src
    ${CMAKE_SOURCE_DIR}/libkss/modules
)

# Source files
set(SOURCES main.cpp)

add_executable(mgsplayer ${SOURCES})

# Handle libkss linking
if(WIN32)
    set(KSS_WIN_BUILD "${CMAKE_SOURCE_DIR}/libkss/build-win")
    target_link_directories(mgsplayer PRIVATE
        ${KSS_WIN_BUILD}
        ${KSS_WIN_BUILD}/modules/kmz80
        ${KSS_WIN_BUILD}/modules/emu2149
        ${KSS_WIN_BUILD}/modules/emu2212
        ${KSS_WIN_BUILD}/modules/emu2413
        ${KSS_WIN_BUILD}/modules/emu8950
        ${KSS_WIN_BUILD}/modules/emu76489
    )
else()
    target_link_directories(mgsplayer PRIVATE
        ${CMAKE_SOURCE_DIR}/libkss/build
        ${CMAKE_SOURCE_DIR}/libkss/build/modules/kmz80
        ${CMAKE_SOURCE_DIR}/libkss/build/modules/emu2149
        ${CMAKE_SOURCE_DIR}/libkss/build/modules/emu2212
        ${CMAKE_SOURCE_DIR}/libkss/build/modules/emu2413
        ${CMAKE_SOURCE_DIR}/libkss/build/modules/emu8950
        ${CMAKE_SOURCE_DIR}/libkss/build/modules/emu76489
    )
endif()

if(WIN32)
    target_link_libraries(mgsplayer PRIVATE 
        ${SDL2_MAIN_LIBRARY}
        ${SDL2_LIBRARIES}
        kss kmz80 emu2149 emu2212 emu2413 emu8950 emu76489
        mingw32 winmm imm32 version ole32 oleaut32 setupapi uuid advapi32 shell32 user32 gdi32
    )
else()
    target_link_libraries(mgsplayer PRIVATE
        ${SDL2_LIBRARIES}
        kss kmz80 emu2149 emu2212 emu2413 emu8950 emu76489
    )
endif()
