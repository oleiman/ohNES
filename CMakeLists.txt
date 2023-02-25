cmake_minimum_required(VERSION 3.0)

if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()
message("BUILD TYPE:  ${CMAKE_BUILD_TYPE}")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wno-long-long  -pedantic")
set(CMAKE_CXX_FLAGS_DEBUG "-g -fsanitize=address -fsanitize=undefined -fno-sanitize-recover=all -fsanitize=float-divide-by-zero -fsanitize=float-cast-overflow -fno-sanitize=null -fno-sanitize=alignment")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

add_subdirectory(external/6502-emu)

project(ohNES)
add_executable(ohNES
  src/main.cpp
  src/system.cpp
  src/ppu_registers.cpp
  src/apu_registers.cpp
  src/cartridge.cpp
  src/mappers/cnrom.cpp
  src/mappers/color_dreams.cpp
  src/mappers/mmc1_2.cpp
  src/mappers/nrom.cpp
  src/mappers/uxrom.cpp
  src/mappers/mmc2.cpp
  src/mappers/mmc3.cpp
  src/mappers/mapper_factory.cpp
  src/sdl/audio.cpp
  src/dbg/nes_debugger.cpp
  src/ppu.cpp
  src/apu.cpp
  src/joypad.cpp
  src/util.cpp
  src/wx_/dbg_app.cpp
  src/wx_/dbg_wxapp.cpp
)

find_package(SDL2 REQUIRED)
# if (LINUX AND NOT APPLE)
#   find_package(PkgConfig REQUIRED)
#   pkg_check_modules(GTK3 REQUIRED gtk+-3.0)
#   target_link_directories(ohNES ${GTK3_LIBRARY_DIRS})
#   target_include_directories(ohNES PRIVATE ${GTK3_LIBRARY_DIRS})
#   add_definitions(${GTK3_CFLAGS_OTHER})
# endif()
if (APPLE)
  find_library(APPKIT_LIB AppKit)
  # target_link_libraries(ohNES PRIVATE ${APPKIT_LIB})
endif()

set(wxWidgets_USE_DEBUG ON)
find_package(wxWidgets REQUIRED core base)

include(${wxWidgets_USE_FILE})

target_include_directories(ohNES PRIVATE
  src
  ${SDL2_INCLUDE_DIRS}
  external/nativefiledialog/src/include external/inc
  ${wxWidgets_INCLUDE_DIRS}
)
target_link_directories(ohNES PRIVATE
  external/nativefiledialog/build/lib/Release/x64
)
set_property(TARGET ohNES PROPERTY CXX_STANDARD 17)

if (LINUX AND NOT APPLE)
set(GPERF_LIB profiler)
endif()

target_link_libraries(ohNES
  6502-emu
  ${SDL2_LIBRARIES}
  ${GPERF_LIB}
  nfd
  ${APPKIT_LIB}
  ${wxWidgets_LIBRARIES}
)
# if (LINUX AND NOT APPLE)
#   target_link_libraries(ohNES ${GTK3_LIBRARIES})
# endif()


if (WITH_TESTS)
  add_subdirectory(test)
endif()
