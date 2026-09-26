#include "launcher_core.hpp"

#include <algorithm>
#include <cctype>
#include <charconv>
#include <iterator>
#include <map>
#include <regex>
#include <sstream>
#include <system_error>

namespace
{
const std::map<std::string, launcher::LogLevel> kLevels = {{"DEBUG", launcher::LogLevel::kDebug},
    {"INFO", launcher::LogLevel::kInfo},
    {"NOTICE", launcher::LogLevel::kNotice},
    {"WARNING", launcher::LogLevel::kWarning},
    {"ERROR", launcher::LogLevel::kError},
    {"PROGRESS", launcher::LogLevel::kProgress}};

const std::string kOutputNamePrefix = "Using output name ";

std::string Trim(const std::string& text)
{
   const auto first = text.find_first_not_of(" \t\r\n");
   if (first == std::string::npos)
   {
      return {};
   }
   const auto last = text.find_last_not_of(" \t\r\n");
   return text.substr(first, last - first + 1);
}

bool FolderExists(const std::filesystem::path& folder)
{
   std::error_code error;
   return !folder.empty() && std::filesystem::is_directory(folder, error);
}
}  // namespace

std::string launcher::ToUtf8(const std::filesystem::path& path)
{
   const auto utf8 = path.generic_u8string();
   std::string text(utf8.size(), '\0');
   std::ranges::transform(utf8, text.begin(), [](const char8_t character) {
      return static_cast<char>(character);
   });
   return text;
}

std::string launcher::MakeConverterConfiguration(const Settings& settings)
{
   // The converter reads quoted strings, and no Windows path can hold a quote, but a mod name can.
   std::string mod_name = settings.mod_name;
   std::erase(mod_name, '"');

   std::ostringstream configuration;
   configuration << "CK3directory = \"" << ToUtf8(settings.ck3_install) << "\"\n";
   configuration << "CK3DocDirectory = \"" << ToUtf8(settings.ck3_documents) << "\"\n";
   configuration << "EU5directory = \"" << ToUtf8(settings.eu5_install) << "\"\n";
   configuration << "targetGameModPath = \"" << ToUtf8(settings.eu5_mods) << "\"\n";
   configuration << "SaveGame = \"" << ToUtf8(settings.save_game) << "\"\n";
   configuration << "output_name = \"" << mod_name << "\"\n";
   configuration << "debug = \"no\"\n";
   return configuration.str();
}

launcher::LogLine launcher::ParseLogLine(const std::string& line)
{
   // The timestamp before the level holds no brackets, so the first pair is always the level.
   const auto open = line.find('[');
   const auto close = open == std::string::npos ? std::string::npos : line.find(']', open);
   if (close == std::string::npos)
   {
      return {LogLevel::kUnknown, Trim(line)};
   }
   const auto level = kLevels.find(line.substr(open + 1, close - open - 1));
   if (level == kLevels.end())
   {
      return {LogLevel::kUnknown, Trim(line)};
   }
   return {level->second, Trim(line.substr(close + 1))};
}

std::optional<int> launcher::ParseProgress(const LogLine& line)
{
   if (line.level != LogLevel::kProgress)
   {
      return std::nullopt;
   }
   int percent = 0;
   const auto* const begin = line.message.data();
   const auto* const end = begin + line.message.size();
   if (const auto result = std::from_chars(begin, end, percent); result.ec != std::errc())
   {
      return std::nullopt;
   }
   return std::clamp(percent, 0, 100);
}

std::optional<std::string> launcher::ParseOutputName(const LogLine& line)
{
   if (line.level != LogLevel::kInfo || !line.message.starts_with(kOutputNamePrefix))
   {
      return std::nullopt;
   }
   auto name = Trim(line.message.substr(kOutputNamePrefix.size()));
   if (name.empty())
   {
      return std::nullopt;
   }
   return name;
}

std::vector<std::filesystem::path> launcher::ParseSteamLibraryFolders(std::istream& vdf)
{
   const std::string contents{std::istreambuf_iterator<char>(vdf), std::istreambuf_iterator<char>()};
   static const std::regex kPathEntry(R"re("path"\s*"([^"]*)")re");

   std::vector<std::filesystem::path> libraries;
   for (auto match = std::sregex_iterator(contents.begin(), contents.end(), kPathEntry);
       match != std::sregex_iterator();
       ++match)
   {
      // Steam escapes the backslashes in Windows paths.
      std::string path = (*match)[1].str();
      for (auto position = path.find(R"(\\)"); position != std::string::npos;
          position = path.find(R"(\\)", position + 1))
      {
         path.erase(position, 1);
      }
      libraries.emplace_back(launcher::FromUtf8(path));
   }
   return libraries;
}

bool launcher::IsGameInstall(const std::filesystem::path& folder)
{
   return FolderExists(folder / "game");
}

std::optional<std::filesystem::path> launcher::FindSteamGame(const std::vector<std::filesystem::path>& libraries,
    const std::string& folder_name)
{
   for (const auto& library: libraries)
   {
      const auto candidate = library / "steamapps" / "common" / folder_name;
      if (IsGameInstall(candidate))
      {
         return candidate;
      }
   }
   return std::nullopt;
}

std::vector<std::string> launcher::ValidateSettings(const Settings& settings)
{
   std::vector<std::string> problems;
   std::error_code error;
   if (settings.save_game.empty())
   {
      problems.emplace_back("Choose the Crusader Kings III save you want to convert.");
   }
   else if (!std::filesystem::is_regular_file(settings.save_game, error))
   {
      problems.emplace_back("The save file can't be found: " + ToUtf8(settings.save_game));
   }
   if (!IsGameInstall(settings.ck3_install))
   {
      problems.emplace_back(
          "Crusader Kings III isn't installed in the folder chosen for it. Choose the folder that "
          "contains its \"game\" folder.");
   }
   if (!FolderExists(settings.ck3_documents))
   {
      problems.emplace_back(
          "The Crusader Kings III documents folder can't be found. It's usually "
          "Documents/Paradox Interactive/Crusader Kings III.");
   }
   if (!IsGameInstall(settings.eu5_install))
   {
      problems.emplace_back(
          "Europa Universalis V isn't installed in the folder chosen for it. Choose the folder that "
          "contains its \"game\" folder.");
   }
   if (settings.eu5_mods.empty())
   {
      problems.emplace_back("Choose where Europa Universalis V keeps its mods.");
   }
   return problems;
}

std::filesystem::path launcher::FromUtf8(const std::string& text)
{
   // std::filesystem only takes UTF-8 through char8_t.
   std::u8string utf8(text.size(), u8'\0');
   std::ranges::transform(text, utf8.begin(), [](const char character) {
      return static_cast<char8_t>(character);
   });
   return utf8;
}

std::vector<int> launcher::VersionNumbers(const std::string& tag)
{
   static const std::regex kVersion(R"((\d+(?:\.\d+)*)\s*$)");
   std::smatch version;
   if (!std::regex_search(tag, version, kVersion))
   {
      return {};
   }
   std::vector<int> numbers;
   std::istringstream parts(version[1].str());
   for (std::string part; std::getline(parts, part, '.');)
   {
      int number = 0;
      std::from_chars(part.data(), part.data() + part.size(), number);
      numbers.push_back(number);
   }
   return numbers;
}

bool launcher::IsNewerRelease(const std::string& candidate, const std::string& current)
{
   auto candidate_numbers = VersionNumbers(candidate);
   auto current_numbers = VersionNumbers(current);
   if (candidate_numbers.empty() || current_numbers.empty())
   {
      return false;
   }
   // 2 and 2.0 are the same version.
   const auto length = std::max(candidate_numbers.size(), current_numbers.size());
   candidate_numbers.resize(length, 0);
   current_numbers.resize(length, 0);
   return candidate_numbers > current_numbers;
}

std::optional<std::string> launcher::NewestReleaseTag(const std::string& releases_json)
{
   // Each release in the list carries "tag_name": "...". Drafts never appear to the public API.
   static const std::regex kTagName(R"re("tag_name"\s*:\s*"([^"]+)")re");
   std::optional<std::string> newest;
   for (auto tag = std::sregex_iterator(releases_json.begin(), releases_json.end(), kTagName);
       tag != std::sregex_iterator();
       ++tag)
   {
      const auto name = (*tag)[1].str();
      if (!VersionNumbers(name).empty() && (!newest.has_value() || IsNewerRelease(name, *newest)))
      {
         newest = name;
      }
   }
   return newest;
}

std::string launcher::ReleaseDisplayName(const std::string& tag)
{
   if (tag.size() > 1 && tag.front() == 'v' && std::isdigit(static_cast<unsigned char>(tag[1])) != 0)
   {
      return "version " + tag.substr(1);
   }
   auto name = tag;
   std::ranges::replace(name, '-', ' ');
   return name;
}
