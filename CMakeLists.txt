cmake_minimum_required(VERSION 3.0)

if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()
message("BUILD TYPE:  ${CMAKE_BUILD_TYPE}")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wno-long-long  -pedantic")
set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

add_subdirectory(external/6502-emu)

project(ohNES)
add_executable(ohNES
  src/main.cpp
  src/system.cpp
  src/ppu_registers.cpp
  src/cartridge.cpp
  # src/mappers/base_mapper.cpp
  src/mappers/cnrom.cpp
  # src/mappers/ppu_map.cpp
  src/mappers/mmc1.cpp
  src/mappers/nrom.cpp
  src/mappers/uxrom.cpp
  src/mappers/mapper_factory.cpp
  src/ppu.cpp
  src/joypad.cpp
  src/util.cpp
)

find_package(SDL2 REQUIRED)
target_include_directories(ohNES PRIVATE inc ${SDL2_INCLUDE_DIRS})
set_property(TARGET ohNES PROPERTY CXX_STANDARD 17)


target_link_libraries(ohNES 6502-emu ${SDL2_LIBRARIES} profiler)

if (WITH_TESTS)
  add_subdirectory(test)
endif()
