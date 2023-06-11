cmake_minimum_required(VERSION 3.0)

if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()
message("BUILD TYPE: ${CMAKE_BUILD_TYPE}")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wno-long-long  -pedantic")
set(CMAKE_CXX_FLAGS_DEBUG "-g -fsanitize=address -fsanitize=undefined -fno-sanitize-recover=all -fsanitize=float-divide-by-zero -fsanitize=float-cast-overflow -fno-sanitize=null -fno-sanitize=alignment")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

add_subdirectory(external/6502-emu)

set(CORE_SRC
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
  src/mappers/axrom.cpp
  src/mappers/mapper_factory.cpp
  src/dbg/nes_debugger.cpp
  src/ppu.cpp
  src/apu.cpp
  src/gen_audio.cpp
  src/joypad.cpp
  src/util.cpp
)

project(ohNES)

add_library(ohNESCore
  ${CORE_SRC}
)
set_property(TARGET ohNESCore PROPERTY CXX_STANDARD 20)
target_include_directories(ohNESCore PUBLIC
  src
)
target_link_libraries(ohNESCore
  6502-emu
)

if (WITH_APP)
  add_executable(ohNES
    src/main.cpp
    src/sdl/audio.cpp
    src/sdl/input.cpp
    src/wx_/dbg_app.cpp
    src/wx_/dbg_wxapp.cpp
  )

  find_package(SDL2 REQUIRED)

  # set(wxWidgets_USE_DEBUG ON)
  find_package(wxWidgets REQUIRED core base adv)

  include(${wxWidgets_USE_FILE})

  target_include_directories(ohNES PRIVATE
    ${SDL2_INCLUDE_DIRS}
    external/inc
    ${wxWidgets_INCLUDE_DIRS}
  )

  set_property(TARGET ohNES PROPERTY CXX_STANDARD 20)

  # if (LINUX AND NOT APPLE)
  #   set(GPERF_LIB profiler)
  # endif()

  target_link_libraries(ohNES
    ohNESCore
    ${SDL2_LIBRARIES}
    ${GPERF_LIB}
    ${wxWidgets_LIBRARIES}
  )
endif()

if (WITH_GTESTS)
  enable_testing()
  add_subdirectory(test)
endif()
