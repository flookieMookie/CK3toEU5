#include "launcher_frame.hpp"

#include <windows.h>
#include <winhttp.h>
#include <wx/fileconf.h>
#include <wx/msw/registry.h>
#include <wx/stdpaths.h>
#include <wx/utils.h>

#include <array>
#include <fstream>
#include <system_error>

namespace
{
const std::filesystem::path kConverterFolder = "CK3toEU5";
const std::filesystem::path kConverterExecutable = "CK3toEU5.exe";
constexpr int kOutputPollMilliseconds = 100;
// The share of the progress bar the conversion itself fills; copying the mod is the rest.
constexpr int kConversionShare = 90;

const wxColour kGood(0, 128, 0);

// Short waits: the check runs while the player is choosing a save, and is simply skipped when
// GitHub can't be reached.
constexpr int kUpdateTimeoutMilliseconds = 5000;

// GitHub's list of the fork's releases, or empty if it couldn't be fetched. Nothing about the
// player is sent: it is a plain request for a public page.
std::string FetchReleasesJson()
{
   const auto to_wide = [](const char* text) {
      return std::wstring(text, text + std::char_traits<char>::length(text));
   };
   std::string body;
   HINTERNET session = WinHttpOpen(L"CK3toEU5-launcher", WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY, WINHTTP_NO_PROXY_NAME,
       WINHTTP_NO_PROXY_BYPASS, 0);
   if (session == nullptr)
   {
      return body;
   }
   WinHttpSetTimeouts(session, kUpdateTimeoutMilliseconds, kUpdateTimeoutMilliseconds, kUpdateTimeoutMilliseconds,
       kUpdateTimeoutMilliseconds);
   HINTERNET connection = WinHttpConnect(session, to_wide(launcher::kReleasesApiHost).c_str(), INTERNET_DEFAULT_HTTPS_PORT, 0);
   HINTERNET request = connection == nullptr ? nullptr
                                             : WinHttpOpenRequest(connection,
                                                   L"GET",
                                                   to_wide(launcher::kReleasesApiPath).c_str(),
                                                   nullptr,
                                                   WINHTTP_NO_REFERER,
                                                   WINHTTP_DEFAULT_ACCEPT_TYPES,
                                                   WINHTTP_FLAG_SECURE);
   DWORD status = 0;
   DWORD status_size = sizeof(status);
   if (request != nullptr &&
       WinHttpSendRequest(request, L"Accept: application/vnd.github+json\r\n", static_cast<DWORD>(-1L), WINHTTP_NO_REQUEST_DATA, 0, 0, 0) &&
       WinHttpReceiveResponse(request, nullptr) &&
       WinHttpQueryHeaders(request, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, WINHTTP_HEADER_NAME_BY_INDEX,
           &status, &status_size, WINHTTP_NO_HEADER_INDEX) &&
       status == 200)
   {
      std::array<char, 8192> chunk{};
      DWORD read = 0;
      while (WinHttpReadData(request, chunk.data(), static_cast<DWORD>(chunk.size()), &read) && read > 0)
      {
         body.append(chunk.data(), read);
      }
   }
   if (request != nullptr)
   {
      WinHttpCloseHandle(request);
   }
   if (connection != nullptr)
   {
      WinHttpCloseHandle(connection);
   }
   WinHttpCloseHandle(session);
   return body;
}
const wxColour kBad(192, 0, 0);
const wxColour kCaution(170, 100, 0);

std::filesystem::path ToPath(const wxString& text)
{
   return {text.ToStdWstring()};
}

wxString ToWx(const std::filesystem::path& path)
{
   return {path.wstring()};
}

// Steam stores its own folder lowercase and with forward slashes. Asking the filesystem for the
// real path gives the folder's actual capitalisation and Windows' own separators.
std::filesystem::path Tidy(const std::filesystem::path& path)
{
   std::error_code error;
   auto tidy = std::filesystem::canonical(path, error);
   if (error)
   {
      tidy = path;
   }
   return tidy.make_preferred();
}

std::filesystem::path LauncherFolder()
{
   return ToPath(wxStandardPaths::Get().GetExecutablePath()).parent_path();
}

// Kept with the player's own settings rather than beside the launcher, so a copy of the launcher's
// folder never carries anyone's paths along with it.
wxString SettingsFile()
{
   return ToWx(ToPath(wxStandardPaths::Get().GetUserConfigDir()) / "CK3toEU5-launcher.ini");
}

// Steam's own folder, from the registry, which every library list hangs off.
std::optional<std::filesystem::path> SteamFolder()
{
   const wxRegKey key(wxRegKey::HKCU, R"(Software\Valve\Steam)");
   wxString steam_path;
   if (key.Exists() && key.QueryValue("SteamPath", steam_path) && !steam_path.empty())
   {
      return ToPath(steam_path);
   }
   std::error_code error;
   const std::filesystem::path default_folder = "C:/Program Files (x86)/Steam";
   if (std::filesystem::is_directory(default_folder, error))
   {
      return default_folder;
   }
   return std::nullopt;
}

std::vector<std::filesystem::path> SteamLibraries()
{
   const auto steam = SteamFolder();
   if (!steam.has_value())
   {
      return {};
   }
   std::vector<std::filesystem::path> libraries{*steam};
   std::ifstream library_list(*steam / "steamapps" / "libraryfolders.vdf");
   if (library_list.is_open())
   {
      const auto listed = launcher::ParseSteamLibraryFolders(library_list);
      libraries.insert(libraries.end(), listed.begin(), listed.end());
   }
   return libraries;
}

std::filesystem::path ParadoxDocuments()
{
   return ToPath(wxStandardPaths::Get().GetDocumentsDir()) / "Paradox Interactive";
}

// Moves everything complete out of a buffer of process output, leaving any partial last line.
std::vector<std::string> TakeCompleteLines(std::string& buffer)
{
   std::vector<std::string> lines;
   for (auto newline = buffer.find('\n'); newline != std::string::npos; newline = buffer.find('\n'))
   {
      lines.emplace_back(buffer.substr(0, newline));
      buffer.erase(0, newline + 1);
   }
   return lines;
}

void ReadAvailable(wxInputStream* stream, std::string& buffer)
{
   if (stream == nullptr)
   {
      return;
   }
   std::array<char, 4096> chunk{};
   // Read returns what is there rather than blocking for a full chunk once CanRead is false.
   while (stream->CanRead())
   {
      stream->Read(chunk.data(), chunk.size());
      const auto count = stream->LastRead();
      if (count == 0)
      {
         break;
      }
      buffer.append(chunk.data(), count);
   }
}
}  // namespace

launcher::LauncherFrame::LauncherFrame():
    wxFrame(nullptr, wxID_ANY, "CK3 to EU5 Converter - " + launcher::ReleaseDisplayName(launcher::kReleaseTag) + " (unofficial)"),
    output_timer_(this)
{
   BuildInterface();
   LoadSettings();
   DetectMissingFolders();
   UpdateFolderStatus();
   StartUpdateCheck();

   Bind(wxEVT_TIMER, &LauncherFrame::OnOutputTimer, this);
   Bind(wxEVT_END_PROCESS, &LauncherFrame::OnProcessEnd, this);
   Bind(wxEVT_CLOSE_WINDOW, &LauncherFrame::OnClose, this);
}

launcher::LauncherFrame::~LauncherFrame()
{
   // The check gives up within its timeouts, so closing never waits long.
   if (update_check_.joinable())
   {
      update_check_.join();
   }
}

void launcher::LauncherFrame::StartUpdateCheck()
{
   update_check_ = std::thread([this]() {
      const auto newest = NewestReleaseTag(FetchReleasesJson());
      if (newest.has_value() && IsNewerRelease(*newest, kReleaseTag))
      {
         CallAfter([this, tag = *newest]() {
            ShowUpdate(tag);
         });
      }
   });
}

void launcher::LauncherFrame::ShowUpdate(const std::string& tag)
{
   update_text_->SetLabel("A new version is available: " + ReleaseDisplayName(tag) + ".");
   update_link_->SetURL(std::string(kReleasePageUrl) + tag);
   update_row_->ShowItems(true);
   panel_->Layout();
}

void launcher::LauncherFrame::BuildInterface()
{
   auto* panel = new wxPanel(this);
   panel_ = panel;
   auto* layout = new wxBoxSizer(wxVERTICAL);
   const int gap = FromDIP(8);
   const auto padded = wxSizerFlags().Expand().Border(wxLEFT | wxRIGHT | wxTOP, FromDIP(12));

   auto* title = new wxStaticText(panel, wxID_ANY, "Convert a Crusader Kings III save into a Europa Universalis V mod");
   auto title_font = title->GetFont();
   title_font.SetPointSize(title_font.GetPointSize() + 3);
   title_font.MakeBold();
   title->SetFont(title_font);
   layout->Add(title, padded);
   auto* subtitle =
       new wxStaticText(panel, wxID_ANY, "Unofficial build. Not made or supported by Paradox Game Converters.");
   subtitle->SetForegroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_GRAYTEXT));
   layout->Add(subtitle, wxSizerFlags().Border(wxLEFT | wxRIGHT, FromDIP(12)));

   // Hidden until the update check finds a newer release.
   update_row_ = new wxBoxSizer(wxHORIZONTAL);
   update_text_ = new wxStaticText(panel, wxID_ANY, "");
   auto update_font = update_text_->GetFont();
   update_font.MakeBold();
   update_text_->SetFont(update_font);
   update_text_->SetForegroundColour(kGood);
   update_row_->Add(update_text_, wxSizerFlags().CenterVertical());
   update_link_ = new wxHyperlinkCtrl(panel, wxID_ANY, "Download it", std::string(kReleasePageUrl) + kReleaseTag);
   update_row_->Add(update_link_, wxSizerFlags().CenterVertical().Border(wxLEFT, FromDIP(8)));
   layout->Add(update_row_, wxSizerFlags().Border(wxLEFT | wxRIGHT | wxTOP, FromDIP(12)));
   update_row_->ShowItems(false);

   auto* save_box = new wxStaticBoxSizer(wxVERTICAL, panel, "1. Your Crusader Kings III save");
   save_picker_ = new wxFilePickerCtrl(save_box->GetStaticBox(),
       wxID_ANY,
       "",
       "Choose the CK3 save to convert",
       "CK3 saves (*.ck3)|*.ck3",
       wxDefaultPosition,
       wxDefaultSize,
       wxFLP_OPEN | wxFLP_FILE_MUST_EXIST | wxFLP_USE_TEXTCTRL);
   save_box->Add(save_picker_, wxSizerFlags().Expand().Border(wxALL, gap));
   auto* save_hint = new wxStaticText(save_box->GetStaticBox(),
       wxID_ANY,
       "EU5 starts on 1 April 1337, so a campaign played to around then fits best, but any save works.");
   save_hint->SetForegroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_GRAYTEXT));
   save_box->Add(save_hint, wxSizerFlags().Border(wxLEFT | wxRIGHT | wxBOTTOM, gap));
   layout->Add(save_box, padded);

   auto* folder_box = new wxStaticBoxSizer(wxVERTICAL, panel, "2. Game folders (found automatically)");
   auto* grid = new wxFlexGridSizer(3, gap, gap);
   grid->AddGrowableCol(1);
   ck3_install_ = AddFolderRow(folder_box->GetStaticBox(), grid, "Crusader Kings III install");
   ck3_documents_ = AddFolderRow(folder_box->GetStaticBox(), grid, "Crusader Kings III documents");
   eu5_install_ = AddFolderRow(folder_box->GetStaticBox(), grid, "Europa Universalis V install");
   eu5_mods_ = AddFolderRow(folder_box->GetStaticBox(), grid, "Europa Universalis V mods");
   folder_box->Add(grid, wxSizerFlags().Expand().Border(wxALL, gap));
   layout->Add(folder_box, padded);

   auto* name_box = new wxStaticBoxSizer(wxVERTICAL, panel, "3. Mod name (optional)");
   mod_name_ = new wxTextCtrl(name_box->GetStaticBox(), wxID_ANY);
   mod_name_->SetHint("Leave empty to name the mod after the save");
   name_box->Add(mod_name_, wxSizerFlags().Expand().Border(wxALL, gap));
   layout->Add(name_box, padded);

   auto* convert_row = new wxBoxSizer(wxHORIZONTAL);
   convert_button_ = new wxButton(panel, wxID_ANY, "Convert");
   auto button_font = convert_button_->GetFont();
   button_font.SetPointSize(button_font.GetPointSize() + 2);
   button_font.MakeBold();
   convert_button_->SetFont(button_font);
   convert_button_->SetMinSize(FromDIP(wxSize(140, 40)));
   convert_row->Add(convert_button_, wxSizerFlags().CenterVertical());
   progress_ = new wxGauge(panel, wxID_ANY, 100);
   convert_row->Add(progress_, wxSizerFlags(1).CenterVertical().Border(wxLEFT, FromDIP(12)));
   layout->Add(convert_row, padded);

   auto* status_row = new wxBoxSizer(wxHORIZONTAL);
   status_ = new wxStaticText(panel, wxID_ANY, "Choose your save, then press Convert.");
   status_row->Add(status_, wxSizerFlags(1).CenterVertical());
   open_mod_folder_ = new wxButton(panel, wxID_ANY, "Open mod folder");
   open_mod_folder_->Hide();
   status_row->Add(open_mod_folder_, wxSizerFlags().CenterVertical());
   layout->Add(status_row, padded);

   show_details_ = new wxCheckBox(panel, wxID_ANY, "Show every step");
   layout->Add(show_details_, padded);
   log_ = new wxTextCtrl(panel,
       wxID_ANY,
       "",
       wxDefaultPosition,
       FromDIP(wxSize(-1, 140)),
       wxTE_MULTILINE | wxTE_READONLY | wxTE_RICH2);
   layout->Add(log_, wxSizerFlags(1).Expand().Border(wxALL, FromDIP(12)));

   convert_button_->Bind(wxEVT_BUTTON, &LauncherFrame::OnConvert, this);
   open_mod_folder_->Bind(wxEVT_BUTTON, &LauncherFrame::OnOpenModFolder, this);
   show_details_->Bind(wxEVT_CHECKBOX, [this](wxCommandEvent& /*event*/) {
      RebuildLog();
   });
   save_picker_->Bind(wxEVT_FILEPICKER_CHANGED, [this](wxFileDirPickerEvent& /*event*/) {
      status_->SetForegroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT));
      status_->SetLabel("Ready. Press Convert.");
   });

   panel->SetSizer(layout);
   auto* frame_layout = new wxBoxSizer(wxVERTICAL);
   frame_layout->Add(panel, wxSizerFlags(1).Expand());
   SetSizerAndFit(frame_layout);
   SetMinSize(FromDIP(wxSize(760, 660)));
   SetSize(FromDIP(wxSize(820, 720)));
   Centre();
}

launcher::LauncherFrame::FolderRow launcher::LauncherFrame::AddFolderRow(wxWindow* parent,
    wxFlexGridSizer* grid,
    const wxString& label)
{
   FolderRow row;
   grid->Add(new wxStaticText(parent, wxID_ANY, label), wxSizerFlags().CenterVertical());
   row.picker = new wxDirPickerCtrl(parent,
       wxID_ANY,
       "",
       "Choose the " + label + " folder",
       wxDefaultPosition,
       wxDefaultSize,
       wxDIRP_USE_TEXTCTRL);
   grid->Add(row.picker, wxSizerFlags().Expand());
   row.status = new wxStaticText(parent, wxID_ANY, "");
   row.status->SetMinSize(FromDIP(wxSize(110, -1)));
   grid->Add(row.status, wxSizerFlags().CenterVertical());
   row.picker->Bind(wxEVT_DIRPICKER_CHANGED, [this](wxFileDirPickerEvent& /*event*/) {
      UpdateFolderStatus();
   });
   return row;
}

void launcher::LauncherFrame::LoadSettings()
{
   const wxFileConfig config("", "", SettingsFile(), "", wxCONFIG_USE_LOCAL_FILE);
   const std::array<std::pair<const char*, wxDirPickerCtrl*>, 4> folders = {{{"ck3_install", ck3_install_.picker},
       {"ck3_documents", ck3_documents_.picker},
       {"eu5_install", eu5_install_.picker},
       {"eu5_mods", eu5_mods_.picker}}};
   wxString value;
   for (const auto& [key, picker]: folders)
   {
      if (config.Read(key, &value) && !value.empty())
      {
         picker->SetPath(value);
      }
   }
   if (config.Read("save_game", &value))
   {
      save_picker_->SetPath(value);
   }
   if (config.Read("mod_name", &value))
   {
      mod_name_->SetValue(value);
   }
}

void launcher::LauncherFrame::SaveSettings() const
{
   wxFileConfig config("", "", SettingsFile(), "", wxCONFIG_USE_LOCAL_FILE);
   config.Write("ck3_install", ck3_install_.picker->GetPath());
   config.Write("ck3_documents", ck3_documents_.picker->GetPath());
   config.Write("eu5_install", eu5_install_.picker->GetPath());
   config.Write("eu5_mods", eu5_mods_.picker->GetPath());
   config.Write("save_game", save_picker_->GetPath());
   config.Write("mod_name", mod_name_->GetValue());
   config.Flush();
}

void launcher::LauncherFrame::DetectMissingFolders()
{
   const auto fill = [](const FolderRow& row, const std::optional<std::filesystem::path>& found) {
      if (row.picker->GetPath().empty() && found.has_value())
      {
         row.picker->SetPath(ToWx(Tidy(*found)));
      }
   };
   const auto libraries = SteamLibraries();
   fill(ck3_install_, FindSteamGame(libraries, "Crusader Kings III"));
   fill(eu5_install_, FindSteamGame(libraries, "Europa Universalis V"));

   const auto documents = ParadoxDocuments();
   fill(ck3_documents_, documents / "Crusader Kings III");
   fill(eu5_mods_, documents / "Europa Universalis V" / "mod");

   save_picker_->SetInitialDirectory(ToWx(ToPath(ck3_documents_.picker->GetPath()) / "save games"));
}

void launcher::LauncherFrame::UpdateFolderStatus()
{
   const auto show = [](const FolderRow& row, const bool found) {
      row.status->SetLabel(found ? wxString(L"\u2713 Found") : wxString(L"\u2717 Not found"));
      row.status->SetForegroundColour(found ? kGood : kBad);
   };
   const auto settings = CollectSettings();
   std::error_code error;
   show(ck3_install_, IsGameInstall(settings.ck3_install));
   show(ck3_documents_, std::filesystem::is_directory(settings.ck3_documents, error));
   show(eu5_install_, IsGameInstall(settings.eu5_install));

   // EU5 only makes its mod folder once a mod has been installed, so a missing one is expected.
   if (std::filesystem::is_directory(settings.eu5_mods, error))
   {
      show(eu5_mods_, true);
   }
   else if (!settings.eu5_mods.empty())
   {
      eu5_mods_.status->SetLabel("Will be created");
      eu5_mods_.status->SetForegroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_GRAYTEXT));
   }
   else
   {
      show(eu5_mods_, false);
   }
   Layout();
}

launcher::Settings launcher::LauncherFrame::CollectSettings() const
{
   Settings settings;
   settings.ck3_install = ToPath(ck3_install_.picker->GetPath());
   settings.ck3_documents = ToPath(ck3_documents_.picker->GetPath());
   settings.eu5_install = ToPath(eu5_install_.picker->GetPath());
   settings.eu5_mods = ToPath(eu5_mods_.picker->GetPath());
   settings.save_game = ToPath(save_picker_->GetPath());
   settings.mod_name = mod_name_->GetValue().Strip(wxString::both).utf8_string();
   return settings;
}

void launcher::LauncherFrame::OnConvert(wxCommandEvent& /*event*/)
{
   const auto settings = CollectSettings();
   if (const auto problems = ValidateSettings(settings); !problems.empty())
   {
      wxString message;
      for (const auto& problem: problems)
      {
         message += wxString(L"\u2022 ") + wxString::FromUTF8(problem) + "\n";
      }
      wxMessageBox(message, "Before converting", wxOK | wxICON_WARNING, this);
      return;
   }
   SaveSettings();

   log_lines_.clear();
   log_->Clear();
   output_buffer_.clear();
   error_buffer_.clear();
   output_name_.reset();
   installed_mod_.clear();
   warning_count_ = 0;
   show_details_->SetLabel("Show every step");
   progress_->SetValue(0);
   open_mod_folder_->Hide();
   status_->SetForegroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT));

   if (!StartConverter(settings))
   {
      return;
   }
   SetRunning(true);
   status_->SetLabel("Converting... this usually takes under a minute.");
   output_timer_.Start(kOutputPollMilliseconds);
}

bool launcher::LauncherFrame::StartConverter(const Settings& settings)
{
   const auto converter_folder = LauncherFolder() / kConverterFolder;
   const auto converter = converter_folder / kConverterExecutable;
   std::error_code error;
   if (!std::filesystem::is_regular_file(converter, error))
   {
      FinishConversion(false,
          "The converter is missing from the launcher's folder. Unzip the whole download again and run the launcher "
          "from there.");
      return false;
   }

   {
      std::ofstream configuration(converter_folder / "configuration.txt", std::ios::binary);
      configuration << MakeConverterConfiguration(settings);
      if (!configuration)
      {
         FinishConversion(false,
             "The converter's settings couldn't be saved. If the launcher is in a protected folder such as Program "
             "Files, move it to your Desktop or Documents.");
         return false;
      }
   }

   process_ = new wxProcess(this);
   process_->Redirect();
   wxExecuteEnv environment;
   environment.cwd = ToWx(converter_folder);
   wxGetEnvMap(&environment.env);
   const wxString command = "\"" + ToWx(converter) + "\"";
   if (wxExecute(command, wxEXEC_ASYNC | wxEXEC_HIDE_CONSOLE, process_, &environment) == 0)
   {
      delete process_;
      process_ = nullptr;
      FinishConversion(false, "The converter couldn't be started.");
      return false;
   }
   return true;
}

void launcher::LauncherFrame::OnOutputTimer(wxTimerEvent& /*event*/)
{
   DrainOutput();
}

void launcher::LauncherFrame::DrainOutput()
{
   if (process_ == nullptr)
   {
      return;
   }
   ReadAvailable(process_->GetInputStream(), output_buffer_);
   ReadAvailable(process_->GetErrorStream(), error_buffer_);
   for (const auto& line: TakeCompleteLines(output_buffer_))
   {
      HandleOutputLine(line);
   }
   for (const auto& line: TakeCompleteLines(error_buffer_))
   {
      HandleOutputLine(line);
   }
}

void launcher::LauncherFrame::HandleOutputLine(const std::string& raw_line)
{
   const auto line = ParseLogLine(raw_line);
   if (line.message.empty())
   {
      return;
   }
   if (const auto percent = ParseProgress(line); percent.has_value())
   {
      progress_->SetValue(*percent * kConversionShare / 100);
      return;
   }
   if (const auto name = ParseOutputName(line); name.has_value())
   {
      output_name_ = name;
   }
   if (line.level == LogLevel::kWarning)
   {
      ++warning_count_;
   }
   AddLogLine(line);
}

void launcher::LauncherFrame::OnProcessEnd(wxProcessEvent& event)
{
   output_timer_.Stop();
   DrainOutput();
   // Whatever is left over had no newline after it, but is still a line.
   if (!output_buffer_.empty())
   {
      HandleOutputLine(output_buffer_);
      output_buffer_.clear();
   }
   if (!error_buffer_.empty())
   {
      HandleOutputLine(error_buffer_);
      error_buffer_.clear();
   }
   // The handler owns the process once it has seen this event, but wxWidgets is still unwinding
   // from it, so the object goes once the event is over.
   auto* finished = process_;
   process_ = nullptr;
   CallAfter([finished] {
      delete finished;
   });

   if (event.GetExitCode() != 0)
   {
      FinishConversion(false, "The conversion failed. The details below say why.");
      return;
   }
   status_->SetLabel("Copying the mod into EU5...");
   progress_->SetValue(kConversionShare);
   if (const auto problem = InstallMod(); !problem.empty())
   {
      FinishConversion(false, "The mod was converted, but couldn't be copied into EU5: " + problem);
      return;
   }
   progress_->SetValue(100);
   FinishConversion(true, "Done! Your mod is in EU5's mod folder.");
}

wxString launcher::LauncherFrame::InstallMod()
{
   if (!output_name_.has_value())
   {
      return "the converter didn't say which mod it made.";
   }
   const auto source = LauncherFolder() / kConverterFolder / "output" / FromUtf8(*output_name_);
   const auto mods_folder = ToPath(eu5_mods_.picker->GetPath());
   const auto destination = mods_folder / FromUtf8(*output_name_);

   std::error_code error;
   if (!std::filesystem::is_directory(source, error))
   {
      return "the converted mod wasn't where the converter left it.";
   }
   std::filesystem::create_directories(mods_folder, error);
   if (error)
   {
      return wxString(error.message());
   }
   // A mod of the same name is an earlier conversion of the same save, which this replaces.
   std::filesystem::remove_all(destination, error);
   if (error)
   {
      return "an earlier copy couldn't be removed. Close EU5 if it's running and try again.";
   }
   std::filesystem::copy(source, destination, std::filesystem::copy_options::recursive, error);
   if (error)
   {
      return wxString(error.message());
   }
   installed_mod_ = destination;
   AddLogLine({LogLevel::kNotice, "Mod copied to " + ToUtf8(destination)});
   return {};
}

void launcher::LauncherFrame::FinishConversion(const bool succeeded, const wxString& message)
{
   SetRunning(false);
   status_->SetLabel(message);
   status_->SetForegroundColour(succeeded ? kGood : kBad);
   open_mod_folder_->Show(succeeded);
   if (warning_count_ > 0)
   {
      show_details_->SetLabel(wxString::Format("Show every step (%d technical warnings)", warning_count_));
   }
   if (!succeeded && !show_details_->IsChecked())
   {
      // The lines leading up to a failure are usually what explains it.
      show_details_->SetValue(true);
      RebuildLog();
   }
   Layout();

   if (succeeded)
   {
      const auto name = wxString::FromUTF8(output_name_.value_or(""));
      wxMessageBox(wxString::Format("Your mod \"%s\" is ready.\n\n"
                                    "To play it:\n"
                                    "1. Start Europa Universalis V.\n"
                                    "2. Click the shield icon on the main menu to open Mods & DLCs.\n"
                                    "3. Enable \"%s\" and start a new game.",
                       name,
                       name),
          "Conversion complete",
          wxOK | wxICON_INFORMATION,
          this);
   }
}

void launcher::LauncherFrame::SetRunning(const bool running)
{
   convert_button_->Enable(!running);
   save_picker_->Enable(!running);
   ck3_install_.picker->Enable(!running);
   ck3_documents_.picker->Enable(!running);
   eu5_install_.picker->Enable(!running);
   eu5_mods_.picker->Enable(!running);
   mod_name_->Enable(!running);
}

void launcher::LauncherFrame::AddLogLine(const LogLine& line)
{
   log_lines_.push_back(line);
   if (IsShownInLog(line))
   {
      WriteLogLine(line);
   }
}

bool launcher::LauncherFrame::IsShownInLog(const LogLine& line) const
{
   if (show_details_->IsChecked())
   {
      return true;
   }
   // The converter's warnings are notes on its own coverage, meant for whoever maintains it. They
   // don't stop a mod from working, so they stay out of the way unless asked for.
   return line.level == LogLevel::kNotice || line.level == LogLevel::kError || line.level == LogLevel::kUnknown;
}

void launcher::LauncherFrame::WriteLogLine(const LogLine& line)
{
   wxString prefix;
   wxColour colour = wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT);
   switch (line.level)
   {
      case LogLevel::kError:
         prefix = "Error: ";
         colour = kBad;
         break;
      case LogLevel::kWarning:
         prefix = "Warning: ";
         colour = kCaution;
         break;
      case LogLevel::kNotice:
         colour = kGood;
         break;
      default:
         break;
   }
   log_->SetDefaultStyle(wxTextAttr(colour));
   log_->AppendText(prefix + wxString::FromUTF8(line.message) + "\n");
}

void launcher::LauncherFrame::RebuildLog()
{
   log_->Freeze();
   log_->Clear();
   for (const auto& line: log_lines_)
   {
      if (IsShownInLog(line))
      {
         WriteLogLine(line);
      }
   }
   log_->Thaw();
   log_->ShowPosition(log_->GetLastPosition());
}

void launcher::LauncherFrame::OnOpenModFolder(wxCommandEvent& /*event*/)
{
   if (!installed_mod_.empty())
   {
      wxLaunchDefaultApplication(ToWx(installed_mod_));
   }
}

void launcher::LauncherFrame::OnClose(wxCloseEvent& event)
{
   if (process_ != nullptr && event.CanVeto())
   {
      wxMessageBox("A conversion is still running. It usually finishes within a minute.",
          "Still converting",
          wxOK | wxICON_INFORMATION,
          this);
      event.Veto();
      return;
   }
   SaveSettings();
   event.Skip();
}
