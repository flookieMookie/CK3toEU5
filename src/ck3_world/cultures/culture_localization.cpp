#include "culture_localization.hpp"

#include <external/commonItems/Log.h>

#include <fstream>
#include <string>
#include <system_error>

commonItems::LocalizationDatabase ck3::LoadCultureLocalization(const std::filesystem::path& ck3_directory)
{
   // The database only reads languages it is told about up front.
   commonItems::LocalizationDatabase localization("english",
       {"french", "german", "japanese", "korean", "polish", "russian", "simp_chinese", "spanish"});
   const auto localization_folder = ck3_directory / "game" / "localization";

   std::error_code error;
   if (!std::filesystem::is_directory(localization_folder, error))
   {
      Log(LogLevel::Warning) << "CK3 localisation not found - converted cultures will be named after their keys.";
      return localization;
   }

   int languages = 0;
   for (const auto& entry: std::filesystem::directory_iterator(localization_folder, error))
   {
      if (!entry.is_directory())
      {
         continue;
      }
      const auto language = entry.path().filename().string();
      std::ifstream file(entry.path() / "culture" / ("cultures_l_" + language + ".yml"));
      if (file.is_open() && localization.ScrapeStream(file) > 0)
      {
         ++languages;
      }
   }
   Log(LogLevel::Info) << "<> Loaded " << localization.size() << " CK3 culture names in " << languages << " languages.";
   return localization;
}
