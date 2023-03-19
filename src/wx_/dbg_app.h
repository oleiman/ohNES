#pragma once

#include "dbg/nes_debugger.hpp"
#include "wx_/dbg_wxapp.h"

#include <thread>

namespace dbg {

class DebuggerApp {
public:
  DebuggerApp(sys::NES &console);

  void StartApp();

  wx_internal::DbgWxApp *App() { return app; }

private:
  int c = 0;
  char **v = nullptr;
  std::thread _app_thread;
  wx_internal::DbgWxApp *app;
};
} // namespace dbg
