#include "launcher_window.hpp"

#include <commctrl.h>
#include <richedit.h>
#include <shellapi.h>
#include <shlobj.h>
#include <shobjidl.h>
#include <winhttp.h>

#include <algorithm>
#include <array>
#include <cwctype>
#include <filesystem>
#include <fstream>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <system_error>
#include <thread>
#include <vector>

#include "launcher_core.hpp"

namespace
{
const std::filesystem::path kConverterFolder = "CK3toEU5";
const std::filesystem::path kConverterExecutable = "CK3toEU5.exe";
// The share of the progress bar the conversion itself fills; copying the mod is the rest.
constexpr int kConversionShare = 90;
// Short waits: the check runs while the player is choosing a save, and is simply skipped when
// GitHub can't be reached.
constexpr int kUpdateTimeoutMilliseconds = 5000;

constexpr COLORREF kGood = RGB(0, 128, 0);
constexpr COLORREF kBad = RGB(192, 0, 0);
constexpr COLORREF kCaution = RGB(170, 100, 0);

// What the background threads tell the window. Text travels as a heap copy the window frees.
constexpr UINT kOutputLineMessage = WM_APP + 1;
constexpr UINT kConverterExitedMessage = WM_APP + 2;
constexpr UINT kUpdateFoundMessage = WM_APP + 3;

constexpr int kSaveBrowseId = 100;
constexpr int kFolderBrowseId = 110;  // one for each folder row, in order
constexpr int kSavePathId = 120;
constexpr int kFolderPathId = 130;  // one for each folder row, in order
constexpr int kConvertId = 140;
constexpr int kOpenModFolderId = 141;
constexpr int kShowDetailsId = 142;
constexpr int kUpdateLinkId = 143;

const wchar_t* const kWindowClass = L"CK3toEU5Launcher";

std::wstring Widen(const std::string& text)
{
   if (text.empty())
   {
      return {};
   }
   const auto length = MultiByteToWideChar(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), nullptr, 0);
   std::wstring wide(static_cast<std::size_t>(length), L'\0');
   MultiByteToWideChar(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), wide.data(), length);
   return wide;
}

std::string Narrow(const std::wstring& text)
{
   if (text.empty())
   {
      return {};
   }
   const auto length =
       WideCharToMultiByte(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), nullptr, 0, nullptr, nullptr);
   std::string narrow(static_cast<std::size_t>(length), '\0');
   WideCharToMultiByte(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), narrow.data(), length, nullptr, nullptr);
   return narrow;
}

std::wstring GetText(HWND control)
{
   std::wstring text(static_cast<std::size_t>(GetWindowTextLengthW(control)) + 1, L'\0');
   text.resize(static_cast<std::size_t>(GetWindowTextW(control, text.data(), static_cast<int>(text.size()))));
   return text;
}

std::wstring Trim(const std::wstring& text)
{
   const auto first = text.find_first_not_of(L" \t\r\n");
   if (first == std::wstring::npos)
   {
      return {};
   }
   return text.substr(first, text.find_last_not_of(L" \t\r\n") - first + 1);
}

// GitHub's list of the fork's releases, or empty if it couldn't be fetched. Nothing about the
// player is sent: it is a plain request for a public page.
std::string FetchReleasesJson()
{
   std::string body;
   HINTERNET session = WinHttpOpen(L"CK3toEU5-launcher",
       WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY,
       WINHTTP_NO_PROXY_NAME,
       WINHTTP_NO_PROXY_BYPASS,
       0);
   if (session == nullptr)
   {
      return body;
   }
   WinHttpSetTimeouts(session,
       kUpdateTimeoutMilliseconds,
       kUpdateTimeoutMilliseconds,
       kUpdateTimeoutMilliseconds,
       kUpdateTimeoutMilliseconds);
   HINTERNET connection =
       WinHttpConnect(session, Widen(launcher::kReleasesApiHost).c_str(), INTERNET_DEFAULT_HTTPS_PORT, 0);
   HINTERNET request = connection == nullptr ? nullptr
                                             : WinHttpOpenRequest(connection,
                                                   L"GET",
                                                   Widen(launcher::kReleasesApiPath).c_str(),
                                                   nullptr,
                                                   WINHTTP_NO_REFERER,
                                                   WINHTTP_DEFAULT_ACCEPT_TYPES,
                                                   WINHTTP_FLAG_SECURE);
   DWORD status = 0;
   DWORD status_size = sizeof(status);
   if (request != nullptr &&
       WinHttpSendRequest(request,
           L"Accept: application/vnd.github+json\r\n",
           static_cast<DWORD>(-1L),
           WINHTTP_NO_REQUEST_DATA,
           0,
           0,
           0) &&
       WinHttpReceiveResponse(request, nullptr) &&
       WinHttpQueryHeaders(request,
           WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
           WINHTTP_HEADER_NAME_BY_INDEX,
           &status,
           &status_size,
           WINHTTP_NO_HEADER_INDEX) &&
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

std::filesystem::path KnownFolder(REFKNOWNFOLDERID id)
{
   PWSTR path = nullptr;
   std::filesystem::path folder;
   if (SUCCEEDED(SHGetKnownFolderPath(id, 0, nullptr, &path)))
   {
      folder = path;
   }
   CoTaskMemFree(path);
   return folder;
}

std::filesystem::path LauncherFolder()
{
   std::wstring path(MAX_PATH, L'\0');
   while (true)
   {
      const auto length = GetModuleFileNameW(nullptr, path.data(), static_cast<DWORD>(path.size()));
      if (length < path.size())
      {
         path.resize(length);
         break;
      }
      path.resize(path.size() * 2);
   }
   return std::filesystem::path(path).parent_path();
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

// Steam's own folder, from the registry, which every library list hangs off.
std::optional<std::filesystem::path> SteamFolder()
{
   std::array<wchar_t, 1024> buffer{};
   auto size = static_cast<DWORD>(buffer.size() * sizeof(wchar_t));
   if (RegGetValueW(HKEY_CURRENT_USER,
           L"Software\\Valve\\Steam",
           L"SteamPath",
           RRF_RT_REG_SZ,
           nullptr,
           buffer.data(),
           &size) == ERROR_SUCCESS &&
       buffer[0] != L'\0')
   {
      return std::filesystem::path(buffer.data());
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

// Kept with the player's own settings rather than beside the launcher, so a copy of the launcher's
// folder never carries anyone's paths along with it. One key = value a line, backslashes doubled,
// as earlier launchers wrote it.
std::filesystem::path SettingsFile()
{
   return KnownFolder(FOLDERID_RoamingAppData) / "CK3toEU5-launcher.ini";
}

std::wstring Unescape(const std::wstring& value)
{
   auto text = value;
   if (text.size() >= 2 && text.front() == L'"' && text.back() == L'"')
   {
      text = text.substr(1, text.size() - 2);
   }
   std::wstring plain;
   for (std::size_t position = 0; position < text.size(); ++position)
   {
      if (text[position] != L'\\' || position + 1 == text.size())
      {
         plain += text[position];
         continue;
      }
      switch (text[++position])
      {
         case L'n':
            plain += L'\n';
            break;
         case L'r':
            plain += L'\r';
            break;
         case L't':
            plain += L'\t';
            break;
         default:
            plain += text[position];
            break;
      }
   }
   return plain;
}

std::wstring Escape(const std::wstring& value)
{
   std::wstring escaped;
   for (const auto character: value)
   {
      if (character == L'\\' || character == L'"')
      {
         escaped += L'\\';
      }
      escaped += character;
   }
   if (!value.empty() && (std::iswspace(value.front()) != 0 || std::iswspace(value.back()) != 0))
   {
      return L"\"" + escaped + L"\"";
   }
   return escaped;
}

std::map<std::string, std::wstring> ReadSettingsFile()
{
   std::map<std::string, std::wstring> values;
   std::ifstream file(SettingsFile(), std::ios::binary);
   std::string line;
   while (std::getline(file, line))
   {
      if (!line.empty() && line.back() == '\r')
      {
         line.pop_back();
      }
      const auto equals = line.find('=');
      if (equals != std::string::npos && !line.starts_with('['))
      {
         values[std::string(Narrow(Trim(Widen(line.substr(0, equals)))))] = Unescape(Widen(line.substr(equals + 1)));
      }
   }
   return values;
}

// A file or folder the player picks with Windows' own dialog, starting where they are likely to look.
std::optional<std::wstring> PickPath(HWND owner,
    const bool folder,
    const std::wstring& title,
    const std::wstring& start)
{
   IFileOpenDialog* dialog = nullptr;
   if (FAILED(CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&dialog))))
   {
      return std::nullopt;
   }
   FILEOPENDIALOGOPTIONS options = 0;
   dialog->GetOptions(&options);
   dialog->SetOptions(options | FOS_FORCEFILESYSTEM | (folder ? FOS_PICKFOLDERS : FOS_FILEMUSTEXIST));
   dialog->SetTitle(title.c_str());
   if (!folder)
   {
      const COMDLG_FILTERSPEC saves{L"CK3 saves (*.ck3)", L"*.ck3"};
      dialog->SetFileTypes(1, &saves);
   }
   std::error_code error;
   if (!start.empty() && std::filesystem::is_directory(start, error))
   {
      IShellItem* item = nullptr;
      if (SUCCEEDED(SHCreateItemFromParsingName(start.c_str(), nullptr, IID_PPV_ARGS(&item))))
      {
         dialog->SetFolder(item);
         item->Release();
      }
   }
   std::optional<std::wstring> chosen;
   IShellItem* result = nullptr;
   if (SUCCEEDED(dialog->Show(owner)) && SUCCEEDED(dialog->GetResult(&result)))
   {
      PWSTR path = nullptr;
      if (SUCCEEDED(result->GetDisplayName(SIGDN_FILESYSPATH, &path)))
      {
         chosen = path;
         CoTaskMemFree(path);
      }
      result->Release();
   }
   dialog->Release();
   return chosen;
}

void PostLine(HWND window, std::string line)
{
   while (!line.empty() && line.back() == '\r')
   {
      line.pop_back();
   }
   auto copy = std::make_unique<std::string>(std::move(line));
   if (PostMessageW(window, kOutputLineMessage, 0, reinterpret_cast<LPARAM>(copy.get())))
   {
      static_cast<void>(copy.release());
   }
}

class LauncherWindow
{
  public:
   explicit LauncherWindow(HINSTANCE instance): instance_(instance) {}
   ~LauncherWindow();
   LauncherWindow(const LauncherWindow&) = delete;
   LauncherWindow& operator=(const LauncherWindow&) = delete;
   LauncherWindow(LauncherWindow&&) = delete;
   LauncherWindow& operator=(LauncherWindow&&) = delete;

   bool Create(int show);
   [[nodiscard]] HWND Handle() const { return handle_; }

   static LRESULT CALLBACK Procedure(HWND window, UINT message, WPARAM wparam, LPARAM lparam);

  private:
   // A folder the converter needs, with a note beside it saying whether it was found.
   struct FolderRow
   {
      HWND label = nullptr;
      HWND path = nullptr;
      HWND browse = nullptr;
      HWND status = nullptr;
      std::wstring name;
   };

   LRESULT HandleMessage(UINT message, WPARAM wparam, LPARAM lparam);
   HWND AddControl(const wchar_t* type, const std::wstring& text, DWORD style, int id = 0, DWORD extended_style = 0);
   void BuildInterface();
   void ApplyFonts();
   void Layout();
   [[nodiscard]] int Scale(int pixels) const { return MulDiv(pixels, static_cast<int>(dpi_), 96); }
   void SetColour(HWND control, COLORREF colour);

   void StartUpdateCheck();
   void ShowUpdate(const std::string& tag);
   void LoadSettings();
   void SaveSettings() const;
   void DetectMissingFolders();
   void UpdateFolderStatus();
   [[nodiscard]] launcher::Settings CollectSettings() const;
   void BrowseForSave();
   void BrowseForFolder(FolderRow& row);

   void OnConvert();
   [[nodiscard]] bool StartConverter(const launcher::Settings& settings);
   void OnConverterExited(DWORD exit_code);
   void HandleOutputLine(const std::string& raw_line);
   void AddLogLine(const launcher::LogLine& line);
   [[nodiscard]] bool IsShownInLog(const launcher::LogLine& line) const;
   void WriteLogLine(const launcher::LogLine& line);
   void RebuildLog();
   // Empty on success, otherwise what went wrong.
   [[nodiscard]] std::wstring InstallMod();
   void FinishConversion(bool succeeded, const std::wstring& message);
   void SetRunning(bool running);
   void SetStatus(const std::wstring& message, COLORREF colour);

   HINSTANCE instance_;
   HWND handle_ = nullptr;
   UINT dpi_ = 96;
   HFONT font_ = nullptr;
   HFONT bold_font_ = nullptr;
   HFONT title_font_ = nullptr;
   HFONT button_font_ = nullptr;
   std::map<HWND, COLORREF> colours_;
   std::vector<HWND> controls_;

   HWND title_ = nullptr;
   HWND subtitle_ = nullptr;
   HWND update_text_ = nullptr;
   HWND update_link_ = nullptr;
   HWND save_group_ = nullptr;
   HWND save_path_ = nullptr;
   HWND save_browse_ = nullptr;
   HWND save_hint_ = nullptr;
   HWND folder_group_ = nullptr;
   std::array<FolderRow, 4> folders_;
   HWND name_group_ = nullptr;
   HWND mod_name_ = nullptr;
   HWND convert_button_ = nullptr;
   HWND progress_ = nullptr;
   HWND status_ = nullptr;
   HWND open_mod_folder_ = nullptr;
   HWND show_details_ = nullptr;
   HWND log_ = nullptr;

   bool loading_ = false;
   bool update_shown_ = false;
   std::wstring update_url_;
   std::thread update_check_;

   bool running_ = false;
   std::thread reader_;
   std::vector<launcher::LogLine> log_lines_;
   std::optional<std::string> output_name_;
   std::filesystem::path installed_mod_;
   int warning_count_ = 0;
};

// The rows of the folder box, in the order they are shown.
enum Folder : std::size_t
{
   kCk3Install,
   kCk3Documents,
   kEu5Install,
   kEu5Mods
};

LauncherWindow::~LauncherWindow()
{
   // The check gives up within its timeouts, so closing never waits long.
   if (update_check_.joinable())
   {
      update_check_.join();
   }
   if (reader_.joinable())
   {
      reader_.join();
   }
   for (auto* font: {font_, bold_font_, title_font_, button_font_})
   {
      if (font != nullptr)
      {
         DeleteObject(font);
      }
   }
}

bool LauncherWindow::Create(const int show)
{
   WNDCLASSEXW window_class{};
   window_class.cbSize = sizeof(window_class);
   window_class.lpfnWndProc = &LauncherWindow::Procedure;
   window_class.hInstance = instance_;
   window_class.hCursor = LoadCursorW(nullptr, IDC_ARROW);
   window_class.hbrBackground = GetSysColorBrush(COLOR_BTNFACE);
   window_class.lpszClassName = kWindowClass;
   window_class.hIcon = LoadIconW(instance_, L"LAUNCHER_ICON");
   window_class.hIconSm = window_class.hIcon;
   RegisterClassExW(&window_class);

   const auto title = L"CK3 to EU5 Converter - " + Widen(launcher::ReleaseDisplayName(launcher::kReleaseTag));
   if (CreateWindowExW(WS_EX_CONTROLPARENT,
           kWindowClass,
           title.c_str(),
           WS_OVERLAPPEDWINDOW,
           CW_USEDEFAULT,
           CW_USEDEFAULT,
           CW_USEDEFAULT,
           CW_USEDEFAULT,
           nullptr,
           nullptr,
           instance_,
           this) == nullptr)
   {
      return false;
   }

   // Sized and centred for the screen it opened on.
   MONITORINFO monitor{};
   monitor.cbSize = sizeof(monitor);
   GetMonitorInfoW(MonitorFromWindow(handle_, MONITOR_DEFAULTTOPRIMARY), &monitor);
   const auto& area = monitor.rcWork;
   const auto width = std::min(Scale(820), static_cast<int>(area.right - area.left));
   const auto height = std::min(Scale(720), static_cast<int>(area.bottom - area.top));
   SetWindowPos(handle_,
       nullptr,
       area.left + (area.right - area.left - width) / 2,
       area.top + (area.bottom - area.top - height) / 2,
       width,
       height,
       SWP_NOZORDER);

   LoadSettings();
   DetectMissingFolders();
   UpdateFolderStatus();
   StartUpdateCheck();
   ShowWindow(handle_, show);
   UpdateWindow(handle_);
   return true;
}

LRESULT CALLBACK LauncherWindow::Procedure(HWND window, const UINT message, const WPARAM wparam, const LPARAM lparam)
{
   if (message == WM_NCCREATE)
   {
      auto* self = static_cast<LauncherWindow*>(reinterpret_cast<CREATESTRUCTW*>(lparam)->lpCreateParams);
      self->handle_ = window;
      self->dpi_ = GetDpiForWindow(window);
      SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
   }
   if (auto* self = reinterpret_cast<LauncherWindow*>(GetWindowLongPtrW(window, GWLP_USERDATA)); self != nullptr)
   {
      return self->HandleMessage(message, wparam, lparam);
   }
   return DefWindowProcW(window, message, wparam, lparam);
}

LRESULT LauncherWindow::HandleMessage(const UINT message, const WPARAM wparam, const LPARAM lparam)
{
   switch (message)
   {
      case WM_CREATE:
         BuildInterface();
         return 0;
      case WM_SIZE:
         Layout();
         return 0;
      case WM_GETMINMAXINFO:
      {
         auto* limits = reinterpret_cast<MINMAXINFO*>(lparam);
         limits->ptMinTrackSize = {Scale(760), Scale(660)};
         return 0;
      }
      case WM_DPICHANGED:
      {
         dpi_ = HIWORD(wparam);
         ApplyFonts();
         const auto* suggested = reinterpret_cast<const RECT*>(lparam);
         SetWindowPos(handle_,
             nullptr,
             suggested->left,
             suggested->top,
             suggested->right - suggested->left,
             suggested->bottom - suggested->top,
             SWP_NOZORDER | SWP_NOACTIVATE);
         Layout();
         return 0;
      }
      case WM_CTLCOLORSTATIC:
      {
         auto* context = reinterpret_cast<HDC>(wparam);
         SetBkColor(context, GetSysColor(COLOR_BTNFACE));
         if (const auto colour = colours_.find(reinterpret_cast<HWND>(lparam)); colour != colours_.end())
         {
            SetTextColor(context, colour->second);
         }
         return reinterpret_cast<LRESULT>(GetSysColorBrush(COLOR_BTNFACE));
      }
      case WM_COMMAND:
      {
         const auto id = static_cast<int>(LOWORD(wparam));
         const auto code = HIWORD(wparam);
         if (code == BN_CLICKED)
         {
            if (id == kConvertId)
            {
               OnConvert();
            }
            else if (id == kOpenModFolderId && !installed_mod_.empty())
            {
               ShellExecuteW(handle_, L"open", installed_mod_.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
            }
            else if (id == kShowDetailsId)
            {
               RebuildLog();
            }
            else if (id == kSaveBrowseId)
            {
               BrowseForSave();
            }
            else if (id >= kFolderBrowseId && id < kFolderBrowseId + static_cast<int>(folders_.size()))
            {
               BrowseForFolder(folders_[static_cast<std::size_t>(id - kFolderBrowseId)]);
            }
         }
         else if (code == EN_CHANGE && !loading_)
         {
            if (id == kSavePathId && !running_)
            {
               SetStatus(L"Ready. Press Convert.", GetSysColor(COLOR_WINDOWTEXT));
            }
            else if (id >= kFolderPathId && id < kFolderPathId + static_cast<int>(folders_.size()))
            {
               UpdateFolderStatus();
            }
         }
         return 0;
      }
      case WM_NOTIFY:
      {
         const auto* notice = reinterpret_cast<const NMHDR*>(lparam);
         if (notice->idFrom == static_cast<UINT_PTR>(kUpdateLinkId) &&
             (notice->code == NM_CLICK || notice->code == NM_RETURN))
         {
            ShellExecuteW(handle_, L"open", update_url_.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
         }
         return 0;
      }
      case kOutputLineMessage:
      {
         const std::unique_ptr<std::string> line(reinterpret_cast<std::string*>(lparam));
         HandleOutputLine(*line);
         return 0;
      }
      case kConverterExitedMessage:
         OnConverterExited(static_cast<DWORD>(wparam));
         return 0;
      case kUpdateFoundMessage:
      {
         const std::unique_ptr<std::string> tag(reinterpret_cast<std::string*>(lparam));
         ShowUpdate(*tag);
         return 0;
      }
      case WM_CLOSE:
         if (running_)
         {
            MessageBoxW(handle_,
                L"A conversion is still running. It usually finishes within a minute.",
                L"Still converting",
                MB_OK | MB_ICONINFORMATION);
            return 0;
         }
         SaveSettings();
         DestroyWindow(handle_);
         return 0;
      case WM_DESTROY:
         PostQuitMessage(0);
         return 0;
      default:
         return DefWindowProcW(handle_, message, wparam, lparam);
   }
}

HWND LauncherWindow::AddControl(const wchar_t* type,
    const std::wstring& text,
    const DWORD style,
    const int id,
    const DWORD extended_style)
{
   HWND control = CreateWindowExW(extended_style,
       type,
       text.c_str(),
       WS_CHILD | WS_VISIBLE | style,
       0,
       0,
       0,
       0,
       handle_,
       reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)),
       instance_,
       nullptr);
   controls_.push_back(control);
   return control;
}

void LauncherWindow::BuildInterface()
{
   title_ = AddControl(L"STATIC", L"Convert a Crusader Kings III save into a Europa Universalis V mod", SS_LEFT);
   subtitle_ = AddControl(L"STATIC",
       L"Built on Paradox Game Converters' CK3toEU5, but not made or supported by them.",
       SS_LEFT);
   SetColour(subtitle_, GetSysColor(COLOR_GRAYTEXT));

   // Hidden until the update check finds a newer release.
   update_text_ = AddControl(L"STATIC", L"", SS_LEFT);
   SetColour(update_text_, kGood);
   update_link_ = AddControl(WC_LINK, L"<a>Download it</a>", WS_TABSTOP, kUpdateLinkId);
   ShowWindow(update_text_, SW_HIDE);
   ShowWindow(update_link_, SW_HIDE);

   save_group_ = AddControl(L"BUTTON", L"1. Your Crusader Kings III save", BS_GROUPBOX);
   save_path_ = AddControl(L"EDIT", L"", ES_AUTOHSCROLL | WS_TABSTOP, kSavePathId, WS_EX_CLIENTEDGE);
   save_browse_ = AddControl(L"BUTTON", L"Browse...", BS_PUSHBUTTON | WS_TABSTOP, kSaveBrowseId);
   save_hint_ = AddControl(L"STATIC",
       L"EU5 starts on 1 April 1337, so a campaign played to around then fits best, but any save works.",
       SS_LEFT);
   SetColour(save_hint_, GetSysColor(COLOR_GRAYTEXT));

   folder_group_ = AddControl(L"BUTTON", L"2. Game folders (found automatically)", BS_GROUPBOX);
   const std::array<const wchar_t*, 4> names = {L"Crusader Kings III install",
       L"Crusader Kings III documents",
       L"Europa Universalis V install",
       L"Europa Universalis V mods"};
   for (std::size_t index = 0; index < folders_.size(); ++index)
   {
      auto& row = folders_[index];
      row.name = names[index];
      row.label = AddControl(L"STATIC", row.name, SS_LEFT | SS_CENTERIMAGE);
      row.path = AddControl(L"EDIT",
          L"",
          ES_AUTOHSCROLL | WS_TABSTOP,
          kFolderPathId + static_cast<int>(index),
          WS_EX_CLIENTEDGE);
      row.browse =
          AddControl(L"BUTTON", L"Browse...", BS_PUSHBUTTON | WS_TABSTOP, kFolderBrowseId + static_cast<int>(index));
      row.status = AddControl(L"STATIC", L"", SS_LEFT | SS_CENTERIMAGE);
   }

   name_group_ = AddControl(L"BUTTON", L"3. Mod name (optional)", BS_GROUPBOX);
   mod_name_ = AddControl(L"EDIT", L"", ES_AUTOHSCROLL | WS_TABSTOP, 0, WS_EX_CLIENTEDGE);
   SendMessageW(mod_name_,
       EM_SETCUEBANNER,
       TRUE,
       reinterpret_cast<LPARAM>(L"Leave empty to name the mod after the save"));

   convert_button_ = AddControl(L"BUTTON", L"Convert", BS_DEFPUSHBUTTON | WS_TABSTOP, kConvertId);
   progress_ = AddControl(PROGRESS_CLASSW, L"", 0);
   SendMessageW(progress_, PBM_SETRANGE32, 0, 100);
   status_ = AddControl(L"STATIC", L"Choose your save, then press Convert.", SS_LEFT);
   open_mod_folder_ = AddControl(L"BUTTON", L"Open mod folder", BS_PUSHBUTTON | WS_TABSTOP, kOpenModFolderId);
   ShowWindow(open_mod_folder_, SW_HIDE);
   show_details_ = AddControl(L"BUTTON", L"Show every step", BS_AUTOCHECKBOX | WS_TABSTOP, kShowDetailsId);
   log_ = AddControl(MSFTEDIT_CLASS,
       L"",
       ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL | WS_VSCROLL | WS_TABSTOP,
       0,
       WS_EX_CLIENTEDGE);
   SendMessageW(log_, EM_SETBKGNDCOLOR, 0, static_cast<LPARAM>(GetSysColor(COLOR_WINDOW)));

   ApplyFonts();
}

void LauncherWindow::ApplyFonts()
{
   NONCLIENTMETRICSW metrics{};
   metrics.cbSize = sizeof(metrics);
   SystemParametersInfoForDpi(SPI_GETNONCLIENTMETRICS, sizeof(metrics), &metrics, 0, dpi_);
   auto normal = metrics.lfMessageFont;
   auto bold = normal;
   bold.lfWeight = FW_BOLD;
   auto title = bold;
   title.lfHeight = normal.lfHeight - MulDiv(3, static_cast<int>(dpi_), 72);
   auto button = bold;
   button.lfHeight = normal.lfHeight - MulDiv(2, static_cast<int>(dpi_), 72);

   const std::array<HFONT, 4> old_fonts = {font_, bold_font_, title_font_, button_font_};
   font_ = CreateFontIndirectW(&normal);
   bold_font_ = CreateFontIndirectW(&bold);
   title_font_ = CreateFontIndirectW(&title);
   button_font_ = CreateFontIndirectW(&button);
   for (auto* control: controls_)
   {
      auto* font = font_;
      if (control == title_)
      {
         font = title_font_;
      }
      else if (control == update_text_)
      {
         font = bold_font_;
      }
      else if (control == convert_button_)
      {
         font = button_font_;
      }
      SendMessageW(control, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
   }
   for (auto* font: old_fonts)
   {
      if (font != nullptr)
      {
         DeleteObject(font);
      }
   }
}

void LauncherWindow::Layout()
{
   RECT client{};
   GetClientRect(handle_, &client);
   const int margin = Scale(12);
   const int gap = Scale(8);
   const int inner = client.right - 2 * margin;
   const int line = Scale(20);
   const int field = Scale(24);
   const int browse = Scale(88);
   const int caption = Scale(20);
   const auto place = [](HWND control, const int x, const int y, const int width, const int height) {
      MoveWindow(control, x, y, std::max(width, 0), std::max(height, 0), TRUE);
   };

   int y = margin;
   place(title_, margin, y, inner, Scale(28));
   y += Scale(28);
   place(subtitle_, margin, y, inner, line);
   y += line;

   if (update_shown_)
   {
      y += gap;
      // The link sits right after the sentence, however long the version name makes it.
      const auto text = GetText(update_text_);
      auto* context = GetDC(update_text_);
      auto* previous = SelectObject(context, bold_font_);
      SIZE extent{};
      GetTextExtentPoint32W(context, text.c_str(), static_cast<int>(text.size()), &extent);
      SelectObject(context, previous);
      ReleaseDC(update_text_, context);
      place(update_text_, margin, y, extent.cx + Scale(4), line);
      place(update_link_, margin + extent.cx + Scale(10), y, Scale(120), line);
      y += line;
   }
   y += Scale(12);

   // 1. The save.
   int group_top = y;
   y += caption;
   place(save_path_, margin + gap, y, inner - 3 * gap - browse, field);
   place(save_browse_, margin + inner - gap - browse, y - Scale(1), browse, field + Scale(2));
   y += field + Scale(4);
   place(save_hint_, margin + gap, y, inner - 2 * gap, line);
   y += line + gap;
   place(save_group_, margin, group_top, inner, y - group_top);
   y += Scale(12);

   // 2. The game folders.
   group_top = y;
   y += caption;
   const int label = Scale(200);
   const int status = Scale(110);
   for (const auto& row: folders_)
   {
      const int path_left = margin + gap + label + gap;
      const int path_width = inner - 2 * gap - label - gap - browse - gap - status - gap;
      place(row.label, margin + gap, y, label, field);
      place(row.path, path_left, y, path_width, field);
      place(row.browse, path_left + path_width + gap, y - Scale(1), browse, field + Scale(2));
      place(row.status, path_left + path_width + gap + browse + gap, y, status, field);
      y += field + gap;
   }
   place(folder_group_, margin, group_top, inner, y - group_top);
   y += Scale(12);

   // 3. The mod's name.
   group_top = y;
   y += caption;
   place(mod_name_, margin + gap, y, inner - 2 * gap, field);
   y += field + gap;
   place(name_group_, margin, group_top, inner, y - group_top);
   y += Scale(12);

   const int button_height = Scale(40);
   place(convert_button_, margin, y, Scale(140), button_height);
   place(progress_, margin + Scale(152), y + (button_height - Scale(22)) / 2, inner - Scale(152), Scale(22));
   y += button_height + Scale(10);

   const int open_width = Scale(140);
   place(status_, margin, y, inner - open_width - gap, Scale(36));
   place(open_mod_folder_, margin + inner - open_width, y, open_width, Scale(28));
   y += Scale(36) + Scale(4);

   place(show_details_, margin, y, inner, line);
   y += line + gap;

   place(log_, margin, y, inner, client.bottom - margin - y);
}

void LauncherWindow::SetColour(HWND control, const COLORREF colour)
{
   colours_[control] = colour;
   InvalidateRect(control, nullptr, TRUE);
}

void LauncherWindow::StartUpdateCheck()
{
   update_check_ = std::thread([window = handle_]() {
      const auto newest = launcher::NewestReleaseTag(FetchReleasesJson());
      if (newest.has_value() && launcher::IsNewerRelease(*newest, launcher::kReleaseTag))
      {
         auto tag = std::make_unique<std::string>(*newest);
         if (PostMessageW(window, kUpdateFoundMessage, 0, reinterpret_cast<LPARAM>(tag.get())))
         {
            static_cast<void>(tag.release());
         }
      }
   });
}

void LauncherWindow::ShowUpdate(const std::string& tag)
{
   SetWindowTextW(update_text_,
       Widen("A new version is available: " + launcher::ReleaseDisplayName(tag) + ".").c_str());
   update_url_ = Widen(std::string(launcher::kReleasePageUrl) + tag);
   update_shown_ = true;
   ShowWindow(update_text_, SW_SHOW);
   ShowWindow(update_link_, SW_SHOW);
   Layout();
   InvalidateRect(handle_, nullptr, TRUE);
}

void LauncherWindow::LoadSettings()
{
   loading_ = true;
   const auto values = ReadSettingsFile();
   const std::array<const char*, 4> keys = {"ck3_install", "ck3_documents", "eu5_install", "eu5_mods"};
   for (std::size_t index = 0; index < folders_.size(); ++index)
   {
      if (const auto value = values.find(keys[index]); value != values.end() && !value->second.empty())
      {
         SetWindowTextW(folders_[index].path, value->second.c_str());
      }
   }
   if (const auto value = values.find("save_game"); value != values.end())
   {
      SetWindowTextW(save_path_, value->second.c_str());
   }
   if (const auto value = values.find("mod_name"); value != values.end())
   {
      SetWindowTextW(mod_name_, value->second.c_str());
   }
   loading_ = false;
}

void LauncherWindow::SaveSettings() const
{
   std::ofstream file(SettingsFile(), std::ios::binary | std::ios::trunc);
   const std::array<const char*, 4> keys = {"ck3_install", "ck3_documents", "eu5_install", "eu5_mods"};
   for (std::size_t index = 0; index < folders_.size(); ++index)
   {
      file << keys[index] << "=" << Narrow(Escape(GetText(folders_[index].path))) << "\r\n";
   }
   file << "save_game=" << Narrow(Escape(GetText(save_path_))) << "\r\n";
   file << "mod_name=" << Narrow(Escape(GetText(mod_name_))) << "\r\n";
}

void LauncherWindow::DetectMissingFolders()
{
   loading_ = true;
   const auto fill = [](const FolderRow& row, const std::optional<std::filesystem::path>& found) {
      if (GetWindowTextLengthW(row.path) == 0 && found.has_value())
      {
         SetWindowTextW(row.path, Tidy(*found).c_str());
      }
   };
   const auto libraries = SteamLibraries();
   fill(folders_[kCk3Install], launcher::FindSteamGame(libraries, "Crusader Kings III"));
   fill(folders_[kEu5Install], launcher::FindSteamGame(libraries, "Europa Universalis V"));

   const auto documents = KnownFolder(FOLDERID_Documents) / "Paradox Interactive";
   fill(folders_[kCk3Documents], documents / "Crusader Kings III");
   fill(folders_[kEu5Mods], documents / "Europa Universalis V" / "mod");
   loading_ = false;
}

void LauncherWindow::UpdateFolderStatus()
{
   const auto show = [this](const FolderRow& row, const bool found) {
      SetWindowTextW(row.status, found ? L"\u2713 Found" : L"\u2717 Not found");
      SetColour(row.status, found ? kGood : kBad);
   };
   const auto settings = CollectSettings();
   std::error_code error;
   show(folders_[kCk3Install], launcher::IsGameInstall(settings.ck3_install));
   show(folders_[kCk3Documents], std::filesystem::is_directory(settings.ck3_documents, error));
   show(folders_[kEu5Install], launcher::IsGameInstall(settings.eu5_install));

   // EU5 only makes its mod folder once a mod has been installed, so a missing one is expected.
   if (std::filesystem::is_directory(settings.eu5_mods, error))
   {
      show(folders_[kEu5Mods], true);
   }
   else if (!settings.eu5_mods.empty())
   {
      SetWindowTextW(folders_[kEu5Mods].status, L"Will be created");
      SetColour(folders_[kEu5Mods].status, GetSysColor(COLOR_GRAYTEXT));
   }
   else
   {
      show(folders_[kEu5Mods], false);
   }
}

launcher::Settings LauncherWindow::CollectSettings() const
{
   launcher::Settings settings;
   settings.ck3_install = Trim(GetText(folders_[kCk3Install].path));
   settings.ck3_documents = Trim(GetText(folders_[kCk3Documents].path));
   settings.eu5_install = Trim(GetText(folders_[kEu5Install].path));
   settings.eu5_mods = Trim(GetText(folders_[kEu5Mods].path));
   settings.save_game = Trim(GetText(save_path_));
   settings.mod_name = Narrow(Trim(GetText(mod_name_)));
   return settings;
}

void LauncherWindow::BrowseForSave()
{
   // Where the current save is, or else CK3's own save folder.
   const std::filesystem::path current = Trim(GetText(save_path_));
   auto start = current.parent_path();
   std::error_code error;
   if (current.empty() || !std::filesystem::is_directory(start, error))
   {
      start = std::filesystem::path(Trim(GetText(folders_[kCk3Documents].path))) / "save games";
   }
   if (const auto chosen = PickPath(handle_, false, L"Choose the CK3 save to convert", start.wstring()))
   {
      SetWindowTextW(save_path_, chosen->c_str());
   }
}

void LauncherWindow::BrowseForFolder(FolderRow& row)
{
   if (const auto chosen = PickPath(handle_, true, L"Choose the " + row.name + L" folder", Trim(GetText(row.path))))
   {
      SetWindowTextW(row.path, chosen->c_str());
   }
}

void LauncherWindow::OnConvert()
{
   const auto settings = CollectSettings();
   if (const auto problems = launcher::ValidateSettings(settings); !problems.empty())
   {
      std::wstring message;
      for (const auto& problem: problems)
      {
         message += L"\u2022 " + Widen(problem) + L"\n";
      }
      MessageBoxW(handle_, message.c_str(), L"Before converting", MB_OK | MB_ICONWARNING);
      return;
   }
   SaveSettings();

   log_lines_.clear();
   SetWindowTextW(log_, L"");
   output_name_.reset();
   installed_mod_.clear();
   warning_count_ = 0;
   SetWindowTextW(show_details_, L"Show every step");
   SendMessageW(progress_, PBM_SETPOS, 0, 0);
   ShowWindow(open_mod_folder_, SW_HIDE);

   if (!StartConverter(settings))
   {
      return;
   }
   SetRunning(true);
   SetStatus(L"Converting... this usually takes under a minute.", GetSysColor(COLOR_WINDOWTEXT));
}

bool LauncherWindow::StartConverter(const launcher::Settings& settings)
{
   const auto converter_folder = LauncherFolder() / kConverterFolder;
   const auto converter = converter_folder / kConverterExecutable;
   std::error_code error;
   if (!std::filesystem::is_regular_file(converter, error))
   {
      FinishConversion(false,
          L"The converter is missing from the launcher's folder. Unzip the whole download again and run the launcher "
          L"from there.");
      return false;
   }

   {
      std::ofstream configuration(converter_folder / "configuration.txt", std::ios::binary);
      configuration << launcher::MakeConverterConfiguration(settings);
      if (!configuration)
      {
         FinishConversion(false,
             L"The converter's settings couldn't be saved. If the launcher is in a protected folder such as Program "
             L"Files, move it to your Desktop or Documents.");
         return false;
      }
   }

   // The converter writes its log to standard output, which comes back through a pipe.
   SECURITY_ATTRIBUTES inheritable{sizeof(SECURITY_ATTRIBUTES), nullptr, TRUE};
   HANDLE output_read = nullptr;
   HANDLE output_write = nullptr;
   if (!CreatePipe(&output_read, &output_write, &inheritable, 0))
   {
      FinishConversion(false, L"The converter couldn't be started.");
      return false;
   }
   SetHandleInformation(output_read, HANDLE_FLAG_INHERIT, 0);

   STARTUPINFOW startup{};
   startup.cb = sizeof(startup);
   startup.dwFlags = STARTF_USESTDHANDLES;
   startup.hStdOutput = output_write;
   startup.hStdError = output_write;
   PROCESS_INFORMATION process{};
   auto command = L"\"" + converter.wstring() + L"\"";
   const auto started = CreateProcessW(converter.c_str(),
       command.data(),
       nullptr,
       nullptr,
       TRUE,
       CREATE_NO_WINDOW,
       nullptr,
       converter_folder.c_str(),
       &startup,
       &process);
   CloseHandle(output_write);
   if (started == FALSE)
   {
      CloseHandle(output_read);
      FinishConversion(false, L"The converter couldn't be started.");
      return false;
   }
   CloseHandle(process.hThread);

   if (reader_.joinable())
   {
      reader_.join();
   }
   reader_ = std::thread([window = handle_, output_read, converter_process = process.hProcess]() {
      std::string buffer;
      std::array<char, 4096> chunk{};
      DWORD read = 0;
      while (ReadFile(output_read, chunk.data(), static_cast<DWORD>(chunk.size()), &read, nullptr) && read > 0)
      {
         buffer.append(chunk.data(), read);
         for (auto newline = buffer.find('\n'); newline != std::string::npos; newline = buffer.find('\n'))
         {
            PostLine(window, buffer.substr(0, newline));
            buffer.erase(0, newline + 1);
         }
      }
      // Whatever is left over had no newline after it, but is still a line.
      if (!buffer.empty())
      {
         PostLine(window, buffer);
      }
      CloseHandle(output_read);
      WaitForSingleObject(converter_process, INFINITE);
      DWORD exit_code = 1;
      GetExitCodeProcess(converter_process, &exit_code);
      CloseHandle(converter_process);
      PostMessageW(window, kConverterExitedMessage, exit_code, 0);
   });
   return true;
}

void LauncherWindow::OnConverterExited(const DWORD exit_code)
{
   if (reader_.joinable())
   {
      reader_.join();
   }
   if (exit_code != 0)
   {
      FinishConversion(false, L"The conversion failed. The details below say why.");
      return;
   }
   SetStatus(L"Copying the mod into EU5...", GetSysColor(COLOR_WINDOWTEXT));
   SendMessageW(progress_, PBM_SETPOS, kConversionShare, 0);
   if (const auto problem = InstallMod(); !problem.empty())
   {
      FinishConversion(false, L"The mod was converted, but couldn't be copied into EU5: " + problem);
      return;
   }
   SendMessageW(progress_, PBM_SETPOS, 100, 0);
   FinishConversion(true, L"Done! Your mod is in EU5's mod folder.");
}

void LauncherWindow::HandleOutputLine(const std::string& raw_line)
{
   const auto line = launcher::ParseLogLine(raw_line);
   if (line.message.empty())
   {
      return;
   }
   if (const auto percent = launcher::ParseProgress(line); percent.has_value())
   {
      SendMessageW(progress_, PBM_SETPOS, static_cast<WPARAM>(*percent * kConversionShare / 100), 0);
      return;
   }
   if (const auto name = launcher::ParseOutputName(line); name.has_value())
   {
      output_name_ = name;
   }
   if (line.level == launcher::LogLevel::kWarning)
   {
      ++warning_count_;
   }
   AddLogLine(line);
}

std::wstring LauncherWindow::InstallMod()
{
   if (!output_name_.has_value())
   {
      return L"the converter didn't say which mod it made.";
   }
   const auto source = LauncherFolder() / kConverterFolder / "output" / launcher::FromUtf8(*output_name_);
   const std::filesystem::path mods_folder = Trim(GetText(folders_[kEu5Mods].path));
   const auto destination = mods_folder / launcher::FromUtf8(*output_name_);

   std::error_code error;
   if (!std::filesystem::is_directory(source, error))
   {
      return L"the converted mod wasn't where the converter left it.";
   }
   std::filesystem::create_directories(mods_folder, error);
   if (error)
   {
      return Widen(error.message());
   }
   // A mod of the same name is an earlier conversion of the same save, which this replaces.
   std::filesystem::remove_all(destination, error);
   if (error)
   {
      return L"an earlier copy couldn't be removed. Close EU5 if it's running and try again.";
   }
   std::filesystem::copy(source, destination, std::filesystem::copy_options::recursive, error);
   if (error)
   {
      return Widen(error.message());
   }
   installed_mod_ = destination;
   AddLogLine({launcher::LogLevel::kNotice, "Mod copied to " + launcher::ToUtf8(destination)});
   return {};
}

void LauncherWindow::FinishConversion(const bool succeeded, const std::wstring& message)
{
   SetRunning(false);
   SetStatus(message, succeeded ? kGood : kBad);
   ShowWindow(open_mod_folder_, succeeded ? SW_SHOW : SW_HIDE);
   if (warning_count_ > 0)
   {
      SetWindowTextW(show_details_,
          (L"Show every step (" + std::to_wstring(warning_count_) + L" technical warnings)").c_str());
   }
   if (!succeeded && SendMessageW(show_details_, BM_GETCHECK, 0, 0) != BST_CHECKED)
   {
      // The lines leading up to a failure are usually what explains it.
      SendMessageW(show_details_, BM_SETCHECK, BST_CHECKED, 0);
      RebuildLog();
   }

   if (succeeded)
   {
      const auto name = Widen(output_name_.value_or(""));
      const auto text = L"Your mod \"" + name +
                        L"\" is ready.\n\n"
                        L"To play it:\n"
                        L"1. Start Europa Universalis V.\n"
                        L"2. Click the shield icon on the main menu to open Mods & DLCs.\n"
                        L"3. Enable \"" +
                        name + L"\" and start a new game.";
      MessageBoxW(handle_, text.c_str(), L"Conversion complete", MB_OK | MB_ICONINFORMATION);
   }
}

void LauncherWindow::SetRunning(const bool running)
{
   running_ = running;
   for (auto* control: {convert_button_, save_path_, save_browse_, mod_name_})
   {
      EnableWindow(control, running ? FALSE : TRUE);
   }
   for (const auto& row: folders_)
   {
      EnableWindow(row.path, running ? FALSE : TRUE);
      EnableWindow(row.browse, running ? FALSE : TRUE);
   }
}

void LauncherWindow::SetStatus(const std::wstring& message, const COLORREF colour)
{
   SetWindowTextW(status_, message.c_str());
   SetColour(status_, colour);
}

void LauncherWindow::AddLogLine(const launcher::LogLine& line)
{
   log_lines_.push_back(line);
   if (IsShownInLog(line))
   {
      WriteLogLine(line);
   }
}

bool LauncherWindow::IsShownInLog(const launcher::LogLine& line) const
{
   if (SendMessageW(show_details_, BM_GETCHECK, 0, 0) == BST_CHECKED)
   {
      return true;
   }
   // The converter's warnings are notes on its own coverage, meant for whoever maintains it. They
   // don't stop a mod from working, so they stay out of the way unless asked for.
   return line.level == launcher::LogLevel::kNotice || line.level == launcher::LogLevel::kError ||
          line.level == launcher::LogLevel::kUnknown;
}

void LauncherWindow::WriteLogLine(const launcher::LogLine& line)
{
   std::wstring prefix;
   auto colour = GetSysColor(COLOR_WINDOWTEXT);
   switch (line.level)
   {
      case launcher::LogLevel::kError:
         prefix = L"Error: ";
         colour = kBad;
         break;
      case launcher::LogLevel::kWarning:
         prefix = L"Warning: ";
         colour = kCaution;
         break;
      case launcher::LogLevel::kNotice:
         colour = kGood;
         break;
      default:
         break;
   }
   // Added at the end in its own colour.
   GETTEXTLENGTHEX length_query{GTL_NUMCHARS | GTL_PRECISE, 1200};
   const auto end = SendMessageW(log_, EM_GETTEXTLENGTHEX, reinterpret_cast<WPARAM>(&length_query), 0);
   SendMessageW(log_, EM_SETSEL, static_cast<WPARAM>(end), end);
   CHARFORMAT2W format{};
   format.cbSize = sizeof(format);
   format.dwMask = CFM_COLOR;
   format.crTextColor = colour;
   SendMessageW(log_, EM_SETCHARFORMAT, SCF_SELECTION, reinterpret_cast<LPARAM>(&format));
   const auto text = prefix + Widen(line.message) + L"\r\n";
   SendMessageW(log_, EM_REPLACESEL, FALSE, reinterpret_cast<LPARAM>(text.c_str()));
   SendMessageW(log_, WM_VSCROLL, SB_BOTTOM, 0);
}

void LauncherWindow::RebuildLog()
{
   SendMessageW(log_, WM_SETREDRAW, FALSE, 0);
   SetWindowTextW(log_, L"");
   for (const auto& line: log_lines_)
   {
      if (IsShownInLog(line))
      {
         WriteLogLine(line);
      }
   }
   SendMessageW(log_, WM_SETREDRAW, TRUE, 0);
   InvalidateRect(log_, nullptr, TRUE);
   SendMessageW(log_, WM_VSCROLL, SB_BOTTOM, 0);
}
}  // namespace

int launcher::RunLauncher(HINSTANCE instance, const int show)
{
   SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
   const INITCOMMONCONTROLSEX controls{sizeof(INITCOMMONCONTROLSEX),
       ICC_STANDARD_CLASSES | ICC_PROGRESS_CLASS | ICC_LINK_CLASS};
   InitCommonControlsEx(&controls);
   // The rich edit control, for the coloured log, lives in a DLL every Windows has.
   LoadLibraryW(L"Msftedit.dll");
   const auto com = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);

   int result = 0;
   {
      LauncherWindow window(instance);
      if (window.Create(show))
      {
         MSG message{};
         while (GetMessageW(&message, nullptr, 0, 0) > 0)
         {
            if (IsDialogMessageW(window.Handle(), &message) == FALSE)
            {
               TranslateMessage(&message);
               DispatchMessageW(&message);
            }
         }
         result = static_cast<int>(message.wParam);
      }
   }
   if (SUCCEEDED(com))
   {
      CoUninitialize();
   }
   return result;
}
