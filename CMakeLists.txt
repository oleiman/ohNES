cmake_minimum_required(VERSION 3.0)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wno-long-long  -pedantic")

add_subdirectory(external/6502-emu)

project(ohNES)
add_executable(ohNES src/nes.cpp src/mapper.cpp src/ppu_registers.cpp src/cartridge.cpp src/ppu.cpp )

find_package(SDL2 REQUIRED)
target_include_directories(ohNES PRIVATE inc ${SDL2_INCLUDE_DIRS})
set_property(TARGET ohNES PROPERTY CXX_STANDARD 17)


target_link_libraries(ohNES 6502-emu ${SDL2_LIBRARIES} profiler)

if (WITH_TESTS)
  add_subdirectory(test)
endif()
