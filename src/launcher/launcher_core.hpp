#ifndef LAUNCHER_LAUNCHER_CORE_H
#define LAUNCHER_LAUNCHER_CORE_H

#include <filesystem>
#include <istream>
#include <optional>
#include <string>
#include <vector>

// The launcher's logic, kept apart from its window so it can be tested without one.
namespace launcher
{

// This release's tag on GitHub. The launcher compares it with the newest release there to tell the
// player when there is an update, so it changes with every release.
inline constexpr const char* kReleaseTag = "preview-2.1";
// Where the fork's releases are published.
inline constexpr const char* kReleasesApiHost = "api.github.com";
inline constexpr const char* kReleasesApiPath = "/repos/flookieMookie/CK3toEU5/releases?per_page=30";
inline constexpr const char* kReleasePageUrl = "https://github.com/flookieMookie/CK3toEU5/releases/tag/";

// Everything the converter needs to know, as the launcher collects it from the player.
struct Settings
{
   std::filesystem::path ck3_install;
   std::filesystem::path ck3_documents;
   std::filesystem::path eu5_install;
   std::filesystem::path eu5_mods;
   std::filesystem::path save_game;
   // Empty lets the converter name the mod after the save.
   std::string mod_name;
};

enum class LogLevel
{
   kDebug,
   kInfo,
   kNotice,
   kWarning,
   kError,
   kProgress,
   kUnknown
};

// One line of the converter's output, e.g. "2026-09-23 18:40:14     [INFO] Using output name X".
struct LogLine
{
   LogLevel level = LogLevel::kUnknown;
   std::string message;
};

// The configuration.txt the converter reads, in its own format.
[[nodiscard]] std::string MakeConverterConfiguration(const Settings& settings);

[[nodiscard]] LogLine ParseLogLine(const std::string& line);
// The percentage a progress line reports.
[[nodiscard]] std::optional<int> ParseProgress(const LogLine& line);
// The folder name the converter settled on for the mod. It rewrites spaces and dashes, so the
// launcher reads the result rather than predicting it.
[[nodiscard]] std::optional<std::string> ParseOutputName(const LogLine& line);

// Steam lists every library folder it installs games into in steamapps/libraryfolders.vdf.
[[nodiscard]] std::vector<std::filesystem::path> ParseSteamLibraryFolders(std::istream& vdf);
// The game's install folder in the first library that has it, recognised by its game subfolder.
[[nodiscard]] std::optional<std::filesystem::path> FindSteamGame(const std::vector<std::filesystem::path>& libraries,
    const std::string& folder_name);

// Whether a folder looks like a Paradox game install, which always has a game subfolder.
[[nodiscard]] bool IsGameInstall(const std::filesystem::path& folder);

// What is wrong with the settings, in words a player can act on. Empty when conversion can start.
[[nodiscard]] std::vector<std::string> ValidateSettings(const Settings& settings);

// The version numbers in a release tag - preview-2.1 is {2, 1}, v3 is {3}. Empty when there are none.
[[nodiscard]] std::vector<int> VersionNumbers(const std::string& tag);
// Whether one release tag is a later version than another: 2.1 after 2, 2.10 after 2.9.
[[nodiscard]] bool IsNewerRelease(const std::string& candidate, const std::string& current);
// The latest release tag in GitHub's list of releases (the JSON its API returns), if any.
[[nodiscard]] std::optional<std::string> NewestReleaseTag(const std::string& releases_json);
// How a release tag reads to a player: preview-2.1 is "preview 2.1".
[[nodiscard]] std::string ReleaseDisplayName(const std::string& tag);

// A path as UTF-8 with forward slashes, the way the converter's configuration expects it.
[[nodiscard]] std::string ToUtf8(const std::filesystem::path& path);
[[nodiscard]] std::filesystem::path FromUtf8(const std::string& text);

}  // namespace launcher

#endif  // LAUNCHER_LAUNCHER_CORE_H
