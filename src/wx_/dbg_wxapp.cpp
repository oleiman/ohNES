#include "dbg/breakpoint.hpp"
#include "dbg/nes_debugger.hpp"
#include "system.hpp"
#include "wx_/dbg_app.h"

#include "wx/wxprec.h"
#include "wx_/dbg_wxapp.h"
#include <thread>

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include <iostream>
#include <memory>
#include <string>
#include <bitset>

IMPLEMENT_APP_NO_MAIN(wx_internal::DbgWxApp)

namespace wx_internal {

// clang-format off
// wxBEGIN_EVENT_TABLE(CPUStateTable, wxDataViewListCtrl)
// wxEVT_PAINT(constants::TABLE_PAINT, CPUStateTable::OnPaint)
// wxEND_EVENT_TABLE();


wxBEGIN_EVENT_TABLE(DebuggerFrame, wxFrame)
  EVT_MENU(constants::ABOUT, DebuggerFrame::OnAbout)
  EVT_MENU(constants::QUIT, DebuggerFrame::OnQuit)
  EVT_BUTTON(constants::BUTTON_PAUSE, DebuggerFrame::OnPause)
  EVT_BUTTON(constants::BUTTON_STEP, DebuggerFrame::OnStep)
  EVT_BUTTON(constants::BUTTON_RUN, DebuggerFrame::OnRun)
  EVT_BUTTON(constants::BUTTON_RESET, DebuggerFrame::OnReset)
  EVT_BUTTON(constants::BUTTON_LOG, DebuggerFrame::OnLogStart)
  EVT_BUTTON(constants::BUTTON_LOG_STOP, DebuggerFrame::OnLogStop)
  EVT_BUTTON(constants::BUTTON_SET_BP, DebuggerFrame::OnSetBreakpoint)
  EVT_BUTTON(constants::BUTTON_DISABLE_BP, DebuggerFrame::OnDisableBreakpoint)
  EVT_CHECKBOX(constants::MUTE_P1, DebuggerFrame::OnMutePulse1)
  EVT_CHECKBOX(constants::MUTE_P2, DebuggerFrame::OnMutePulse2)
  EVT_CHECKBOX(constants::MUTE_TR, DebuggerFrame::OnMuteTriangle)
  EVT_CHECKBOX(constants::MUTE_NS, DebuggerFrame::OnMuteNoise)
wxEND_EVENT_TABLE();
// clang-format on

bool DbgWxApp::OnInit() {
  if (!wxApp::OnInit()) {
    return false;
  }

  frame = new DebuggerFrame();

  frame->Fit();

  frame->Show(false);

  return true;
}

void CPUStateTable::OnPaint(wxPaintEvent &event) {
  if (_console == nullptr) {
    return;
  }
  const auto &st = _console->state();
  std::stringstream ss;
  ss << "0x" << std::hex << std::setfill('0') << std::setw(2) << +st.rA;
  SetCellValue(0, 0, ss.str());
  ss.str(std::string());

  ss << "0x" << std::hex << std::setfill('0') << std::setw(2) << +st.rX;
  SetCellValue(1, 0, ss.str());
  ss.str(std::string());

  ss << "0x" << std::hex << std::setfill('0') << std::setw(2) << +st.rY;
  SetCellValue(2, 0, ss.str());
  ss.str(std::string());

  ss << "0x" << std::hex << std::setfill('0') << std::setw(2) << +st.sp;
  SetCellValue(3, 0, ss.str());
  ss.str(std::string());

  ss << "0x" << std::hex << std::setfill('0') << std::setw(4) << +st.pc;
  SetCellValue(4, 0, ss.str());
  ss.str(std::string());

  ss << "0b" << std::bitset<8>(st.status);
  SetCellValue(5, 0, ss.str());
  ss.str(std::string());

  ss << std::dec << +st.cycle;
  SetCellValue(6, 0, ss.str());
  ss.str(std::string());
}

DebuggerFrame::DebuggerFrame() : wxFrame(NULL, wxID_ANY, "ohNES CPU debugger") {
  //
  wxMenu *help_menu = new wxMenu();
  help_menu->Append(constants::ABOUT, "&About", "About NES CPU debugger...");

  wxMenuBar *menu_bar = new wxMenuBar();
  menu_bar->Append(help_menu, "&Help");

  SetMenuBar(menu_bar);

  CreateStatusBar();
  SetStatusText("CPU State and Debugger Function");

  wxPanel *p = new wxPanel(this, wxID_ANY);

  wxBoxSizer *topsizer = new wxBoxSizer(wxVERTICAL);

  // topsizer->Add(
  //     new wxStaticText(p, wxID_ANY, "Hello, world!"),
  //     wxSizerFlags().Align(wxALIGN_RIGHT).Border(wxALL & ~wxBOTTOM, 5));

  wxBoxSizer *button_box = new wxBoxSizer(wxHORIZONTAL);
  button_box->Add(new wxButton(p, constants::BUTTON_PAUSE, "Pause"),
                  wxSizerFlags().Border(wxALL, 7));
  button_box->Add(new wxButton(p, constants::BUTTON_STEP, "Step"),
                  wxSizerFlags().Border(wxALL, 7));
  button_box->Add(new wxButton(p, constants::BUTTON_RUN, "Run"),
                  wxSizerFlags().Border(wxALL, 7));
  button_box->Add(new wxButton(p, constants::BUTTON_RESET, "Reset"),
                  wxSizerFlags().Border(wxALL, 7));

  wxBoxSizer *button_box2 = new wxBoxSizer(wxHORIZONTAL);
  button_box2->Add(new wxButton(p, constants::BUTTON_LOG, "Start Log"),
                   wxSizerFlags().Border(wxALL, 7));
  button_box2->Add(new wxButton(p, constants::BUTTON_LOG_STOP, "Stop Log"),
                   wxSizerFlags().Border(wxALL, 7));
  button_box2->Add(new wxButton(p, constants::BUTTON_SET_BP, "Set BP"),
                   wxSizerFlags().Border(wxALL, 7));
  button_box2->Add(new wxButton(p, constants::BUTTON_DISABLE_BP, "Disable BP"),
                   wxSizerFlags().Border(wxALL, 7));

  topsizer->Add(button_box, wxSizerFlags().Center());
  topsizer->Add(button_box2, wxSizerFlags().Center());

  _trace_win = new TraceScrollWindow(p);

  wxSizer *sizerScrollWin = new wxBoxSizer(wxHORIZONTAL);
  sizerScrollWin->Add(_trace_win, wxSizerFlags(1).Expand().Border(wxALL, 7));

  _state_table = new CPUStateTable(p);
  _bp_list = new BreakpointScrollWindow(p);
  _bp_list->SetToolTip("BREAKPOINTS");

  wxSizer *sizerCPUStateWin = new wxBoxSizer(wxHORIZONTAL);
  sizerCPUStateWin->Add(_state_table,
                        wxSizerFlags(1).Expand().Border(wxALL, 7));
  sizerCPUStateWin->Add(_bp_list, wxSizerFlags(1).Expand().Border(wxALL, 7));

  wxBoxSizer *muteBoxSizer = new wxBoxSizer(wxVERTICAL);
  muteBoxSizer->Add(new wxCheckBox(p, constants::MUTE_P1, "Mute Pulse 1"),
                    wxSizerFlags().Border(wxALL, 7));
  muteBoxSizer->Add(new wxCheckBox(p, constants::MUTE_P2, "Mute Pulse 2"),
                    wxSizerFlags().Border(wxALL, 7));
  muteBoxSizer->Add(new wxCheckBox(p, constants::MUTE_TR, "Mute Triangle"),
                    wxSizerFlags().Border(wxALL, 7));
  muteBoxSizer->Add(new wxCheckBox(p, constants::MUTE_NS, "Mute Noise"),
                    wxSizerFlags().Border(wxALL, 7));

  sizerCPUStateWin->Add(muteBoxSizer,
                        wxSizerFlags(1).Expand().Border(wxALL, 7));

  sizerScrollWin->Add(sizerCPUStateWin, wxSizerFlags(1).Expand());

  topsizer->Add(sizerScrollWin, wxSizerFlags(1).Expand());

  p->SetSizer(topsizer);

  _trace_win->ShowScrollbars(wxSHOW_SB_NEVER, wxSHOW_SB_ALWAYS);
  _bp_list->ShowScrollbars(wxSHOW_SB_NEVER, wxSHOW_SB_ALWAYS);

  _bp_dialog = new wxTextEntryDialog(p, "PC", "Set Breakpoint");
  _bp_dialog->SetValue("Enter an address in PRG memory");

  _bp_disable = new wxNumberEntryDialog(p, "Select", "Index",
                                        "Disable Breakpoint", -1, -1, 31);

  topsizer->Fit(this);
}

void DebuggerFrame::OnQuit(wxCommandEvent &WXUNUSED(event)) { Close(true); }
void DebuggerFrame::OnAbout(wxCommandEvent &WXUNUSED(event)) {
  (void)wxMessageBox("A debug interface for the NES cpu\n",
                     "About NES CPU debugger", wxOK | wxICON_INFORMATION);
}

void DebuggerFrame::OnPause(wxCommandEvent &event) {
  _console->debugger().pause(true);
  Refresh();
}
void DebuggerFrame::OnStep(wxCommandEvent &event) {
  _console->debugger().stepOne();
  Refresh();
}
void DebuggerFrame::OnRun(wxCommandEvent &event) {
  _console->debugger().pause(false);
}

void DebuggerFrame::OnReset(wxCommandEvent &event) { _console->reset(true); }

void DebuggerFrame::OnLogStart(wxCommandEvent &event) {
  _console->debugger().setLogging(true);
}
void DebuggerFrame::OnLogStop(wxCommandEvent &event) {
  _console->debugger().setLogging(false);
}
void DebuggerFrame::OnSetBreakpoint(wxCommandEvent &event) {
  _bp_dialog->ShowModal();
  try {
    auto addr = std::stoi(std::string(_bp_dialog->GetValue()), nullptr, 16);
    if (addr <= 0xFFFF) {
      _console->debugger().setBreakpoint<sys::PcBreakpoint>(
          true, static_cast<uint16_t>(addr));
    }
    Refresh();
  } catch (std::invalid_argument &e) {
    std::cerr << e.what() << std::endl;
  }
}
void DebuggerFrame::OnDisableBreakpoint(wxCommandEvent &event) {
  _bp_disable->ShowModal();
  auto i = _bp_disable->GetValue();
  if (i >= 0) {
    _console->debugger().disableBreakpoint(i);
  }
  Refresh();
}

void DebuggerFrame::OnMutePulse1(wxCommandEvent &event) {
  _console->debugger().muteApuChannel(0, event.IsChecked());
}
void DebuggerFrame::OnMutePulse2(wxCommandEvent &event) {
  _console->debugger().muteApuChannel(1, event.IsChecked());
}
void DebuggerFrame::OnMuteTriangle(wxCommandEvent &event) {
  _console->debugger().muteApuChannel(2, event.IsChecked());
}
void DebuggerFrame::OnMuteNoise(wxCommandEvent &event) {
  _console->debugger().muteApuChannel(3, event.IsChecked());
}

void TraceScrollWindow::OnDraw(wxDC &dc) {
  int y = 0;
  sys::NESDebugger::AddressT pc_offset = 0;
  for (int line = -n_lines / 4; line < n_lines; ++line) {
    if (line < 0) {
      dc.SetTextForeground(*wxRED);
    } else if (line > 0) {
      dc.SetTextForeground(*wxBLACK);
    } else {
      dc.SetTextForeground(*wxBLUE);
    }

    int yPhys;
    CalcScrolledPosition(0, y, NULL, &yPhys);
    std::stringstream ss;
    if (line >= 0) {
      instr::Instruction in(console->debugger().decode(pc_offset));
      pc_offset += in.size;
      ss << console->debugger().InstrToStr(in);
    } else {
      ss << console->debugger().InstrToStr(console->debugger().history(line));
    }
    dc.DrawText(wxString::Format("%s", ss.str().c_str()), 0, y);
    y += line_height;
  }
}

void BreakpointScrollWindow::OnDraw(wxDC &dc) {
  int y = 0;
  const auto &bps = console->debugger().breakpoints();
  for (int i = 0; i < bps.size(); ++i) {
    const auto &bp = *bps[i];
    if (bp.isEnabled()) {
      dc.SetTextForeground(*wxRED);
    } else {
      dc.SetTextForeground(*wxBLUE);
    }
    int yPhys;
    CalcScrolledPosition(0, y, NULL, &yPhys);
    std::stringstream ss;
    ss << std::uppercase << i << " | " << bp.str();
    dc.DrawText(wxString::Format("%s", ss.str().c_str()), 0, y);
    y += line_height;
  }
}

} // namespace wx_internal
