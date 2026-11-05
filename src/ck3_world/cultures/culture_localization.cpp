#include "culture_localization.hpp"

#include <external/commonItems/Log.h>

#include <algorithm>
#include <cctype>
#include <fstream>
#include <string>
#include <system_error>
#include <vector>

namespace
{
// Reads one family of CK3 localisation files - localization/<language>/<folder>/<stem>_l_<language>.yml,
// and the files beside it that end the same way, as chinese_dynasty_names_l_english.yml does - in every
// language CK3 ships, then the matching files of the save's mods, which override it.
commonItems::LocalizationDatabase LoadCK3Localization(const std::filesystem::path& ck3_directory,
    const std::string& folder,
    const std::string& stem,
    const std::string& description,
    const std::vector<Mod>& mods,
    const std::string& mod_path_keyword)
{
   // The database only reads languages it is told about up front.
   commonItems::LocalizationDatabase localization("english",
       {"french", "german", "japanese", "korean", "polish", "russian", "simp_chinese", "spanish"});
   const auto localization_folder = ck3_directory / "game" / "localization";

   std::error_code error;
   if (!std::filesystem::is_directory(localization_folder, error))
   {
      Log(LogLevel::Warning) << "CK3 localisation not found - converted " << description
                             << " will be named after their keys.";
   }

   int languages = 0;
   for (const auto& entry: std::filesystem::directory_iterator(localization_folder, error))
   {
      if (!entry.is_directory())
      {
         continue;
      }
      const auto language = entry.path().filename().string();
      const auto ending = stem + "_l_" + language + ".yml";
      // The main file first, so any of the others that names a key again has the last word.
      std::vector<std::filesystem::path> files;
      for (const auto& candidate: std::filesystem::directory_iterator(entry.path() / folder, error))
      {
         if (const auto name = candidate.path().filename().string();
             candidate.is_regular_file() && name.ends_with(ending) && name != ending)
         {
            files.push_back(candidate.path());
         }
      }
      std::ranges::sort(files);
      files.insert(files.begin(), entry.path() / folder / ending);
      bool scraped = false;
      for (const auto& path: files)
      {
         std::ifstream file(path);
         scraped = (file.is_open() && localization.ScrapeStream(file) > 0) || scraped;
      }
      languages += scraped ? 1 : 0;
   }

   // Mods name their files as they like, but nearly always after the family, as CK3 does, or in a
   // folder of that name.
   int mod_files = 0;
   for (const auto& mod: mods)
   {
      const auto mod_localization = mod.path / "localization";
      if (!std::filesystem::is_directory(mod_localization, error))
      {
         continue;
      }
      for (const auto& entry: std::filesystem::recursive_directory_iterator(mod_localization, error))
      {
         auto relative = std::filesystem::relative(entry.path(), mod_localization, error).generic_string();
         std::ranges::transform(relative, relative.begin(), [](const unsigned char character) {
            return static_cast<char>(std::tolower(character));
         });
         if (!entry.is_regular_file() || entry.path().extension() != ".yml" || !relative.contains(mod_path_keyword))
         {
            continue;
         }
         std::ifstream file(entry.path());
         if (file.is_open() && localization.ScrapeStream(file) > 0)
         {
            ++mod_files;
         }
      }
   }

   Log(LogLevel::Info) << "<> Loaded " << localization.size() << " CK3 " << description << " names in " << languages
                       << " languages, " << mod_files << " file(s) of them from mods.";
   return localization;
}
}  // namespace

commonItems::LocalizationDatabase ck3::LoadCultureLocalization(const std::filesystem::path& ck3_directory,
    const std::vector<Mod>& mods)
{
   return LoadCK3Localization(ck3_directory, "culture", "cultures", "culture", mods, "cultur");
}

commonItems::LocalizationDatabase ck3::LoadDynastyLocalization(const std::filesystem::path& ck3_directory,
    const std::vector<Mod>& mods)
{
   return LoadCK3Localization(ck3_directory, "dynasties", "dynasty_names", "dynasty", mods, "dynast");
}

commonItems::LocalizationDatabase ck3::LoadNicknameLocalization(const std::filesystem::path& ck3_directory,
    const std::vector<Mod>& mods)
{
   // Nicknames sit directly in each language's folder: localization/english/nicknames_l_english.yml.
   return LoadCK3Localization(ck3_directory, "", "nicknames", "nickname", mods, "nick");
}
