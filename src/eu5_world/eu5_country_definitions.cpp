#include "eu5_country_definitions.hpp"

#include <string>

#include "CommonRegexes.h"
#include "Log.h"
#include "Parser.h"
#include "ParserHelpers.h"

namespace
{
// game/in_game/setup/countries, relative to the EU5 install root.
const std::filesystem::path kCountriesFolder = std::filesystem::path("game") / "in_game" / "setup" / "countries";
}  // namespace

eu5::CountryDefinitions::CountryDefinitions(const std::filesystem::path& eu5_directory)
{
   const auto countries_folder = eu5_directory / kCountriesFolder;
   if (!std::filesystem::exists(countries_folder))
   {
      Log(LogLevel::Warning) << "EU5 country definitions not found at " << countries_folder.string()
                             << " - converted tags cannot be validated.";
      return;
   }

   for (const auto& entry: std::filesystem::directory_iterator(countries_folder))
   {
      if (entry.is_regular_file() && entry.path().extension() == ".txt")
      {
         LoadFile(entry.path());
      }
   }
   Log(LogLevel::Info) << "<> Loaded " << tags_.size() << " EU5 country tags.";
}

void eu5::CountryDefinitions::LoadFile(const std::filesystem::path& file_path)
{
   commonItems::parser parser;
   parser.registerRegex(R"([A-Z0-9]{3})", [this](const std::string& tag, std::istream& input_stream) {
      tags_.insert(tag);
      commonItems::ignoreItem(tag, input_stream);
   });
   parser.registerRegex(commonItems::catchallRegex, commonItems::ignoreItem);
   parser.parseFile(file_path);
   parser.clearRegisteredKeywords();
}
