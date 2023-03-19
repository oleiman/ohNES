#include "dbg_app.h"
#include "wx_/dbg_wxapp.h"

namespace dbg {
DebuggerApp::DebuggerApp(sys::NES &console) {
  app = new wx_internal::DbgWxApp();
  wxApp::SetInstance(app);
  if (!wxEntryStart(c, v)) {
    return;
  }
  wxTheApp->CallOnInit();
  app->SetConsole(&console);
  _app_thread = std::thread(std::bind(&DebuggerApp::StartApp, this));
  _app_thread.detach();
  app->Show(true);
}

void DebuggerApp::StartApp() {
  wxTheApp->OnRun();
  wxTheApp->OnExit();
  wxEntryCleanup();
}
} // namespace dbg
