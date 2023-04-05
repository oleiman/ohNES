#pragma once

#include "input.hpp"

#include "SDL.h"

#include <iostream>

namespace sdl_internal {

class MoviePlayer {
public:
  static constexpr uint32_t END_FRAME = 0xFFFFFFFF;

  MoviePlayer(const std::string &fname) {
    if (!fname.empty()) {
      std::cerr << "MOVIE: '" << fname << "'";
      m_stream_ = std::ifstream(fname, std::ios::in | std::ios::binary);
      if (!m_stream_) {
        std::cerr << " not found...";
      }
      std::cerr << std::endl;
    }
    // TODO(oren): add a header of some kind to validate movie-ness of the
    // specified file
  }

  void generateEvents() {
    if (!m_stream_ || m_stream_.eof()) {
      return;
    }
    uint32_t next = 0;
    m_stream_.read(reinterpret_cast<char *>(&next), sizeof(next));

    while (!m_stream_.eof() && next != END_FRAME) {
      SDL_Event event;
      SDL_zero(event);
      event.type = sdl_internal::RecordingInputHandler::EventType();
      event.user.code = next;
      auto res = SDL_PushEvent(&event) == 1;
      assert(res != 0);
      if (res == 1) {
        m_stream_.read(reinterpret_cast<char *>(&next), sizeof(next));
      } else {
        std::cerr << "MoviePlayer ERROR: " << SDL_GetError() << std::endl;
      }
    }
  }

private:
  std::ifstream m_stream_;
};

} // namespace sdl_internal
