#ifndef LAUNCHER_LAUNCHER_FRAME_H
#define LAUNCHER_LAUNCHER_FRAME_H

#include <wx/filepicker.h>
#include <wx/process.h>
#include <wx/timer.h>
#include <wx/wx.h>

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

#include "launcher_core.hpp"

namespace launcher
{

// The launcher's one window. The player picks a save, checks the game folders it found, and
// presses Convert; the converter runs in the background and the finished mod is copied into
// EU5's mod folder.
class LauncherFrame: public wxFrame
{
  public:
   LauncherFrame();

  private:
   // A folder the converter needs, with a note beside it saying whether it was found.
   struct FolderRow
   {
      wxDirPickerCtrl* picker = nullptr;
      wxStaticText* status = nullptr;
   };

   void BuildInterface();
   FolderRow AddFolderRow(wxWindow* parent, wxFlexGridSizer* grid, const wxString& label);
   void LoadSettings();
   void SaveSettings() const;
   void DetectMissingFolders();
   void UpdateFolderStatus();
   [[nodiscard]] Settings CollectSettings() const;

   void OnConvert(wxCommandEvent& event);
   void OnOutputTimer(wxTimerEvent& event);
   void OnProcessEnd(wxProcessEvent& event);
   void OnOpenModFolder(wxCommandEvent& event);
   void OnClose(wxCloseEvent& event);

   [[nodiscard]] bool StartConverter(const Settings& settings);
   void DrainOutput();
   void HandleOutputLine(const std::string& raw_line);
   void AddLogLine(const LogLine& line);
   [[nodiscard]] bool IsShownInLog(const LogLine& line) const;
   void WriteLogLine(const LogLine& line);
   void RebuildLog();
   // Empty on success, otherwise what went wrong.
   [[nodiscard]] wxString InstallMod();
   void FinishConversion(bool succeeded, const wxString& message);
   void SetRunning(bool running);

   wxFilePickerCtrl* save_picker_ = nullptr;
   FolderRow ck3_install_;
   FolderRow ck3_documents_;
   FolderRow eu5_install_;
   FolderRow eu5_mods_;
   wxTextCtrl* mod_name_ = nullptr;
   wxButton* convert_button_ = nullptr;
   wxGauge* progress_ = nullptr;
   wxStaticText* status_ = nullptr;
   wxButton* open_mod_folder_ = nullptr;
   wxCheckBox* show_details_ = nullptr;
   wxTextCtrl* log_ = nullptr;

   wxTimer output_timer_;
   wxProcess* process_ = nullptr;
   std::string output_buffer_;
   std::string error_buffer_;
   std::vector<LogLine> log_lines_;
   std::optional<std::string> output_name_;
   std::filesystem::path installed_mod_;
   int warning_count_ = 0;
};

}  // namespace launcher

#endif  // LAUNCHER_LAUNCHER_FRAME_H
