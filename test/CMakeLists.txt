cmake_minimum_required(VERSION 3.14)

project(test_ohnes)

function(test_target)
  # set(options IS_ACTIVATED)
  set(oneValueArgs NAME)
  set(multiValueArgs SRC ROMS)
  cmake_parse_arguments(TEST_TARGET "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  # message("TEST TARGET: ${TEST_TARGET_NAME} ${TEST_TARGET_SRC} ${TEST_TARGET_ROMS}")

  add_executable(
    ${TEST_TARGET_NAME}
    ${TEST_TARGET_SRC}
  )

  target_link_libraries(
    ${TEST_TARGET_NAME}
    ohNESCore
    GTest::gtest_main
  )

  foreach(romfile ${TEST_TARGET_ROMS})
    add_custom_command(
      TARGET ${TEST_TARGET_NAME} POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy
      ${NES_TEST_ROMS}/${romfile}
      ${BIN_ROMS}
    )
  endforeach()

  set(TARGETS "${TARGETS};${TEST_TARGET_NAME}" PARENT_SCOPE)

endfunction()

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_CXX_FLAGS
  -DSOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}
)

include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/837f2229a905f44678fd6dc9af83f9a9f08e4a8f.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
# set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()

set(NES_TEST_ROMS ${CMAKE_SOURCE_DIR}/test/nes-test-roms)
set(BIN_ROMS ${CMAKE_CURRENT_BINARY_DIR}/rom/)
file(MAKE_DIRECTORY ${BIN_ROMS})

test_target(
  NAME cpu_test
  SRC src/cpu_tests.cpp
  ROMS blargg_nes_cpu_test5/cpu.nes
       blargg_nes_cpu_test5/official.nes
       other/nestest.nes
       instr_timing/rom_singles/*.nes
       instr_test-v5/rom_singles/*.nes
       cpu_dummy_writes/*.nes
       cpu_reset/*.nes
)

test_target(
  NAME apu_test
  SRC src/apu_tests.cpp
  ROMS apu_test/rom_singles/*.nes
       # apu_mixer/*.nes

)

test_target(
  NAME ppu_test
  SRC src/ppu_tests.cpp
  ROMS ppu_vbl_nmi/rom_singles/*.nes
  ROMS ppu_read_buffer/test_ppu_read_buffer.nes
)

test_target(
  NAME mapper_test
  SRC src/mapper_tests.cpp
  ROMS mmc3_test_2/rom_singles/*.nes
)

add_custom_target(test
  COMMAND ./cpu_test
  COMMAND ./ppu_test
  COMMAND ./apu_test
  COMMAND ./mapper_test
)

message("TEST TARGETS: ${TARGETS}")
include(GoogleTest)
gtest_discover_tests(${TARGETS})




