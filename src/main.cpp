#include "dbg/nes_debugger.hpp"
#include "ppu.hpp"
#include "sdl/audio.hpp"
#include "sdl/display.hpp"
#include "sdl/input.hpp"
#include "system.hpp"
#include "wx_/dbg_app.h"

#include <SDL.h>
#include <args.hxx>

#include "time.h"
#include <chrono>
#include <iostream>
#include <string>
#include <thread>

using vid::LoadSystemPalette;

using dbg::DebuggerApp;
using sdl_internal::Audio;
using sdl_internal::ControllerInputHandler;
using sdl_internal::Display;
using sdl_internal::KeyboardInputHandler;
using sys::NES;

#define SCALE 2
#define SCREEN_DELAY 2000

const char DEFAULT_PALETTE[] = "../data/2c02.palette";

int main(int argc, char **argv) {
  args::ArgumentParser argparse("Another NES emulator");
  args::HelpFlag help(argparse, "help", "Display this help menu",
                      {'h', "help"});
  args::Group required(argparse, "\nRequired:", args::Group::Validators::All);
  args::Positional<std::string> romfile(required, "romfile",
                                        "iNES file to load");

  args::Group debugging(argparse, "Debugging:");
  args::Flag debug(debugging, "", "CPU debugger", {"debug"});
  args::Flag ppu_debug(debugging, "", "PPU debugger", {"ppu-debug"});
  args::Flag brk(debugging, "", "Immediately break", {'b', "break"});
  args::Flag log(debugging, "", "Enable instruction logging", {"log"});

  try {
    argparse.ParseCLI(argc, argv);
  } catch (args::Help) {
    std::cout << argparse;
    return 0;
  } catch (args::ParseError e) {
    std::cerr << e.what() << std::endl << std::endl;
    std::cerr << argparse;
    return 1;
  } catch (args::ValidationError e) {
    std::cerr << e.what() << std::endl;
    std::cerr << argparse;
    return 1;
  }

  NES nes(romfile.Get(), debug.Get() || log.Get());
  std::unique_ptr<DebuggerApp> cpu_debugger;
  if (debug.Get()) {
    cpu_debugger = std::make_unique<DebuggerApp>(nes);
    if (brk.Get()) {
      nes.debugger().pause(true);
    }
  }

  nes.debugger().setLogging(log.Get());

  LoadSystemPalette(DEFAULT_PALETTE);

  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER |
               SDL_INIT_GAMECONTROLLER) < 0) {
    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "SDL_Init Error",
                             SDL_GetError(), NULL);
    SDL_Delay(2000);
    exit(1);
  }
  {

    KeyboardInputHandler::Init();
    ControllerInputHandler::Init();

    std::unique_ptr<Display<sys::DBG_W, sys::DBG_H>> ppu_debugger = nullptr;

    if (ppu_debug.Get()) {
      ppu_debugger = std::make_unique<Display<sys::DBG_W, sys::DBG_H>>(
          "DBG", "Debug: " + romfile.Get());
    }

    auto display = std::make_unique<Display<vid::WIDTH, vid::HEIGHT, SCALE>>(
        "NES", romfile.Get());

    auto audio = std::make_unique<Audio>();
    audio->init();

    SDL_Event event;
    bool quit = false;
    auto clockStart = std::chrono::steady_clock::now();
    std::chrono::steady_clock::time_point clockEnd;
    while (!quit) {
      while (SDL_PollEvent(&event) != 0) {
        // handle window events
        display->handleEvent(event);
        if (ppu_debugger != nullptr) {
          ppu_debugger->handleEvent(event);
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
            if (cpu_debugger != nullptr) {
              cpu_debugger->App()->Show(true);
            }
            if (ppu_debugger != nullptr) {
              ppu_debugger->focus();
            }
            break;
          case SDLK_p:
            nes.debugger().cyclePalete();
            break;
          case SDLK_r:
            nes.reset(true);
            break;
          case SDLK_1:
          case SDLK_2:
          case SDLK_3:
          case SDLK_4:
            nes.debugger().selectNametable(event.key.keysym.sym - SDLK_1);
            break;
          default:
            break;
          }
        case SDL_KEYUP:
          KeyboardInputHandler::HandleEvent(event, nes,
                                            display->hasMouseFocus());
          break;
        case SDL_CONTROLLERDEVICEADDED:
        case SDL_CONTROLLERDEVICEREMOVED:
        case SDL_CONTROLLERBUTTONDOWN:
        case SDL_CONTROLLERBUTTONUP:
          ControllerInputHandler::HandleEvent(event, nes,
                                              display->hasMouseFocus());
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
      if (ppu_debugger != nullptr && ppu_debugger->isShown()) {
        nes.debugger().render(ppu_debugger->renderBuf);
        ppu_debugger->update();
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
