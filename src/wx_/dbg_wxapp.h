#pragma once

#include "dbg/nes_debugger.hpp"
#include "wx/grid.h"
#include "wx/numdlg.h"
#include "wx/wxprec.h"

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

namespace wx_internal {

class TraceScrollWindow : public wxScrolled<wxWindow> {
public:
  TraceScrollWindow(wxWindow *parent)
      : wxScrolled<wxWindow>(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                             wxBORDER_SUNKEN) {
    wxClientDC dc(this);
    dc.GetTextExtent("NOP", NULL, &line_height);
    SetScrollbars(0, line_height, 0, n_lines + 1, 0, 0, true);
    SetFont(wxFontInfo(12).Family(wxFONTFAMILY_TELETYPE));
  }

  virtual void OnDraw(wxDC &dc) override;

  void SetConsole(sys::NES *console) { this->console = console; }

private:
  int line_height;
  int n_lines = 32;
  sys::NES *console;
};

class BreakpointScrollWindow : public wxScrolled<wxWindow> {
public:
  BreakpointScrollWindow(wxWindow *parent)
      : wxScrolled<wxWindow>(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                             wxBORDER_SUNKEN) {
    SetFont(wxFontInfo(14).Family(wxFONTFAMILY_TELETYPE));
    wxClientDC dc(this);
    dc.GetTextExtent("NOP", NULL, &line_height);
    SetScrollbars(0, line_height, 0, n_lines + 1, 0, 0, true);
  }

  virtual void OnDraw(wxDC &dc) override;

  void SetConsole(sys::NES *console) { this->console = console; }

private:
  int line_height;
  int n_lines = 32;
  sys::NES *console;
};

// class ChannelMutes : public wxWindow {
// public:
//   ChannelMutes(wxWindow *parent)
//       : wxWindow(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize,
//                  wxBORDER_SUNKEN) {
//     SetFont(wxFontInfo(14).Family(wxFONTFAMILY_TELETYPE));

//     // wxClientDC dc(this);
//     // dc.GetTextExtent("NOP", NULL, &line_height);
//     mP1 = new wxCheckBox(this, wxID_ANY
//   }

// private:
//   wxCheckBox *mP1;
//   wxCheckBox *mP2;
//   wxCheckBox *mTr;
//   wxCheckBox *mNs;
// };

class CPUStateText : public wxStaticText {
public:
  CPUStateText(wxWindow *parent, const std::string &label)
      : wxStaticText(parent, wxID_ANY, label, wxDefaultPosition, wxDefaultSize,
                     wxBORDER_SUNKEN) {
    SetFont(wxFontInfo(12).Family(wxFONTFAMILY_TELETYPE));
  }

  void SetConsole(sys::NES *console) { _console = console; }

  // virtual void OnDraw(wxDC &dc) override;
private:
  sys::NES *_console;
};

class CPUStateTable : public wxGrid {
public:
  CPUStateTable(wxWindow *parent)
      : wxGrid(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize) {
    SetFont(wxFontInfo(12).Family(wxFONTFAMILY_TELETYPE));
    Bind(wxEVT_PAINT, &CPUStateTable::OnPaint, this);
    CreateGrid(row_labels.size(), 1);
    SetDefaultCellAlignment(wxALIGN_RIGHT, wxALIGN_CENTER);
    SetDefaultCellBackgroundColour(wxColour(0, 0, 0, wxALPHA_TRANSPARENT));
    SetGridLineColour(*wxLIGHT_GREY);

    SetColSize(0, 100);
    HideColLabels();
    for (int i = 0; i < row_labels.size(); ++i) {
      SetCellFont(i, 0, wxFontInfo(12).Family(wxFONTFAMILY_TELETYPE));
      SetRowLabelValue(i, row_labels[i]);
      SetRowLabelAlignment(wxALIGN_RIGHT, wxALIGN_CENTER);
    }
  }

  void SetConsole(sys::NES *console) { _console = console; }

  void OnPaint(wxPaintEvent &WXUNUSED(event));

private:
  sys::NES *_console;
  std::array<std::string, 7> row_labels{"A",  "X",      "Y",    "SP",
                                        "PC", "status", "cycle"};
};

class DebuggerFrame : public wxFrame {
public:
  DebuggerFrame();

  void OnAbout(wxCommandEvent &event);
  void OnQuit(wxCommandEvent &event);

  void OnPause(wxCommandEvent &event);
  void OnStep(wxCommandEvent &event);
  void OnRun(wxCommandEvent &event);
  void OnReset(wxCommandEvent &event);
  void OnLogStart(wxCommandEvent &event);
  void OnLogStop(wxCommandEvent &event);
  void OnSetBreakpoint(wxCommandEvent &event);
  void OnDisableBreakpoint(wxCommandEvent &event);
  void OnMutePulse1(wxCommandEvent &event);
  void OnMutePulse2(wxCommandEvent &event);
  void OnMuteTriangle(wxCommandEvent &event);
  void OnMuteNoise(wxCommandEvent &event);

  void SetConsole(sys::NES *console) {
    _console = console;
    _trace_win->SetConsole(console);
    _state_table->SetConsole(console);
    _bp_list->SetConsole(console);
  }

private:
  wxDECLARE_EVENT_TABLE();
  TraceScrollWindow *_trace_win;
  CPUStateTable *_state_table;
  BreakpointScrollWindow *_bp_list;
  wxTextEntryDialog *_bp_dialog;
  wxNumberEntryDialog *_bp_disable;
  sys::NES *_console;
};

class DbgWxApp : public wxApp {

public:
  DbgWxApp() {}
  bool OnInit() override;

  void SetNESDebugger(sys::NESDebugger *nes_dbg) { _nes_dbg = nes_dbg; }

  void Show(bool s) { frame->Show(s); }
  void SetConsole(sys::NES *console) { frame->SetConsole(console); }

private:
  DebuggerFrame *frame = nullptr;
  sys::NESDebugger *_nes_dbg = nullptr;
};

namespace constants {
enum {
  // LAYOUT_TEST_SIZER = 101,
  // LAYOUT_TEST_NB_SIZER,
  // LAYOUT_TEST_GB_SIZER,
  // LAYOUT_TEST_PROPORTIONS,
  // LAYOUT_TEST_SET_MINIMAL,
  // LAYOUT_TEST_NESTED,
  // LAYOUT_TEST_WRAP,
  BUTTON_PAUSE = wxID_HIGHEST + 1,
  BUTTON_STEP,
  BUTTON_RUN,
  BUTTON_RESET,
  BUTTON_LOG,
  BUTTON_LOG_STOP,
  BUTTON_SET_BP,
  BUTTON_DISABLE_BP,
  MUTE_P1,
  MUTE_P2,
  MUTE_TR,
  MUTE_NS,
  TABLE_PAINT,
  QUIT = wxID_EXIT,
  ABOUT = wxID_ABOUT,
};
} // namespace constants

} // namespace wx_internal
