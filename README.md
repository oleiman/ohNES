# ohNES

An NES emulator implemented in C++ 17 for fun and Tetris.

Latest phase of development was done primarily on Mac OS w/ `clang-14`, but `gcc` on Linux should also work. 

In particular, the visual debugger functionality has not been tested on Linux and SDL window scaling will probably not look very nice there OOB.

Some of this code is probably a little nuts.

## Dependencies

- `SDL 2.x`
- `wxWidgets` (for CPU debugger GUI)
  - `core` and `base` modules only
- Debug mode builds will set various `AddressSanitizer` flags.
- `Gtest` (fetched automatically)
- `cmake v3.0`

## Building

```
$ git submodule update --init --recursive
$ build.sh \
    wx_ROOT=/path/to/wxWidgets \
    app=<ON|OFF> \
    test=<OFF|ON> \
    mode=<Release|Debug>
```

## Testing

Core emulator functionality is completely isolated from SDL and wxWidgets. In fact, you can build the tests without having either installed.

### All tests

```
$ cd build
$ make test
```

### Individual tests

```
$ cd build/test
$ ls *_test
apu_test cpu_test general_test mapper_test ppu_test
```

## Usage

```
$ ./build/ohNES --help
  ./build/ohNES [romfile] {OPTIONS}

    Another NES emulator

  OPTIONS:

      -h, --help                        Display this help menu
      Required:
        romfile                           iNES file to load
      --movie                           Control recording playback
      Debugging:
        --debug                           CPU debugger
        --ppu-debug                       PPU debugger
        -b, --break                       Immediately break
        --log                             Enable instruction logging
        --record                          Enable controller recording
```

## Features

- Supports both keyboard and USB controller input (via SDL)
- Support for recording controller input live during play and playing back those recordings.
- CPU debugger
  - Add/disable breakpoints
  - View current CPU state
  - Pause/Step/Resume/Reset execution
  - Instruction logging (to file)
  - Limited instruction history/memory lookahead from current PC.
- PPU debugger
  - Nametable
  - Sprites
  - Pattern tables
  - Palettes
  - Live updated controller (pad 1 only)
- Support for the most common mappers (does not detect variants):
  - 0, 1, 2, 3, 4, 7, 9, 11

## 6502-emu

https://github.com/oleiman/6502-emu

ohNES is built around a 6502 emulator core "library" that is more or less freestanding. I use "library" is big scare quotes because it is probably not particularly suited to use outside of my own projects. It is both fast and accurate, but it exports all of its headers and I regularly push straight to master. At some point (not this week or next but maybe this year) I'd like to clean it up at the interface.

## Test notes

Mostly blargg tests and mostly pulled from https://github.com/christopherpow/nes-test-roms.git

- [X] `blargg_nes_cpu_test5`
- [X] `nestest.nes`
- [X] `instr_timing`
- [X] `instr_test-v5`
- [X] `cpu_dummy_writes`
- [X] `cpu_reset`
- [X] `cpu_exec_space`
- [X] `instr_misc`
- [X] `cpu_interrupts_v2`
- [X] `apu_test`
- [X] `apu_mixer`
- [X] `apu_reset`
- [X] `ppu_vbl_nmi`
- [X] `ppu_open_bus`
- [X] `ppu_read_buffer`
- [X] `oam_read`
- [X] `oam_stress`
- [ ] `mmc3_test_2`
  - [X] `1-clocking.nes`
  - [X] `2-details.nes`
  - [X] `3-A12_clocking.nes`
  - [ ] `4-scanline_timing.nes`
  - [X] `5-MMC3.nes`
  - [ ] `6-MMC3_alt.nes`
- [X] Internal Data Structures

Only real criteria for inclusion relate to easy automation:

- Test can be driven to completion without complicated user input
- Detect termination by inspecting memory or register(s)
- Detect status by inspecting memory or register(s)

The `sprite_hit_tests_2005.10.05/` suite doesn't meet any of these criteria, but I've run it manually without issue.

## TODO

- [ ] Sprite overflow implementation
- [ ] APU mixer (currently implemented as a linear sum in SDL)
- [ ] Generally poor sounding APU. This is due to my lack of DSP knowledge (and laziness). I'd like to do a free-standing implementation at some point.
- [ ] Second joypad read (low hanging)
- [ ] Emu configuration GUI (will require extending wx usage beyond debugger)
- [ ] Detect and implement mapper variants 
  - particularly for MMC3 because I want to play Batman: The Game
- [ ] MMC5
- [ ] Web frontend??
- [ ] NTSC output filter? (or use somebody else's)
