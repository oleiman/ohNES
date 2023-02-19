#include "dbg/nes_debugger.hpp"
#include "ppu.hpp"
#include "sdl/display.hpp"
#include "sdl/pad_maps.hpp"
#include "system.hpp"
#include "wx_/dbg_app.h"

#include <SDL.h>
#include <args.hxx>
#include <nfd.h>

#include "time.h"
#include <chrono>
#include <iostream>
#include <string>
#include <thread>

using vid::LoadSystemPalette;

using dbg::DebuggerApp;
using sdl_internal::Display;
using sdl_internal::Kbd2JoyPad;
using sys::NES;

#define SCALE 2
#define SCREEN_DELAY 2000

const char DEFAULT_PALETTE[] = "../data/2c02.palette";

int main(int argc, char **argv) {
  std::string file;
  if (argc >= 2) {
    file = argv[1];
  } else {
    // TODO(oren): appkit version of NFD doesn't seem to work. I'd guess this
    // has something to do with arm/x64 compatibility?
    nfdchar_t *outPath = nullptr;
    nfdresult_t result = NFD_OpenDialog(nullptr, nullptr, &outPath);
    if (result == NFD_OKAY) {
      std::cerr << "Rom Path: " << outPath << std::endl;
      file = outPath;
      free(outPath);
    } else if (result == NFD_CANCEL) {
      std::cerr << "Fun Police" << std::endl;
      exit(0);
    } else {
      std::cerr << "File select error: " << NFD_GetError() << std::endl;
      exit(1);
    }
  }

  NES nes(file);
  // nes.debug_ = true;
  std::unique_ptr<DebuggerApp> debug_app = std::make_unique<DebuggerApp>(nes);

  LoadSystemPalette(DEFAULT_PALETTE);

  if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "SDL_Init Error",
                             SDL_GetError(), NULL);
    SDL_Delay(2000);
    exit(1);
  }
  {

    std::unique_ptr<Display<sys::DBG_W, sys::DBG_H>> debug_display = nullptr;
    // debug_display = std::make_unique<Display<sys::DBG_W, sys::DBG_H>>(
    //     "DBG", "Debug: " + file);

    auto display =
        std::make_unique<Display<vid::WIDTH, vid::HEIGHT, SCALE>>("NES", file);

    SDL_Event event;
    bool quit = false;
    auto clockStart = std::chrono::steady_clock::now();
    std::chrono::steady_clock::time_point clockEnd;
    while (!quit) {
      while (SDL_PollEvent(&event) != 0) {
        // handle window events
        display->handleEvent(event);
        if (nes.debug_) {
          debug_display->handleEvent(event);
        }

        switch (event.type) {
        case SDL_QUIT:
          quit = true;
          break;
        case SDL_KEYDOWN:
          switch (event.key.keysym.sym) {
          case SDLK_g:
            display->focus();
            break;
          case SDLK_d:
            // TODO(oren): do this initialization elsewhere, D should just flip
            // the flag
            if (debug_display == nullptr) {
              debug_display = std::make_unique<Display<sys::DBG_W, sys::DBG_H>>(
                  "DBG", "Debug: " + file);
              debug_app->App()->Show(true);
              nes.debug_ = true;
            } else {
              debug_app->App()->Show(true);
              debug_display->focus();
            }
            break;
          case SDLK_p:
            nes.debugger().cyclePalete();
            break;
          case SDLK_1:
            nes.debugger().selectNametable(0);
            break;
          case SDLK_2:
            nes.debugger().selectNametable(1);
            break;
          case SDLK_3:
            nes.debugger().selectNametable(2);
            break;
          case SDLK_4:
            nes.debugger().selectNametable(3);
            break;
          default:
            break;
          }
          // falls through to check for joypad events
        case SDL_KEYUP: {
          if (display->hasMouseFocus()) {
            auto sym = event.key.keysym.sym;
            auto jp = Kbd2JoyPad.find(sym);
            if (jp != Kbd2JoyPad.end()) {
              auto b = jp->second;
              if (event.type == SDL_KEYDOWN) {
                nes.joypad_1.press(b);
              } else {
                nes.joypad_1.release(b);
              }
            }
          }
        } break;

        default:
          break;
        }
      }

      // Render a whole frame before checking keyboard events. This
      // effectively locks the polling loop to vsync. It's responsive enough.
      do {
        try {
          nes.step();
          if (nes.paused()) {
            break;
          }
        } catch (std::exception &e) {
          clockEnd = std::chrono::steady_clock::now();
          std::cerr << e.what() << std::endl;
          std::cerr << "Cycle: " << nes.state().cycle << std::endl;
          std::cerr << std::hex << "PC: 0x" << +nes.state().pc << std::dec
                    << std::endl;
          quit = true;
          SDL_Delay(SCREEN_DELAY);
          break;
        }
      } while (!nes.render(display->renderBuf));

      display->update();
      if (nes.debug_ && debug_display->isShown()) {
        nes.debugger().render(debug_display->renderBuf);
        debug_display->update();
      }

      if (!display->isShown()) {
        quit = true;
      }
    }

    clockEnd = std::chrono::steady_clock::now();

    std::cerr << "Quitting: "
              << static_cast<double>(display->frames()) /
                     (std::chrono::duration_cast<std::chrono::seconds>(
                          clockEnd - clockStart)
                          .count())
              << "fps" << std::endl;

    std::cout << "Cycles: " << +nes.state().cycle << std::endl;
    std::cout << "Frames: " << display->frames() << std::endl;
  }
  SDL_Quit();

  return 0;
}
