#include "eu5_game_definitions.hpp"

#include <string>

#include "CommonRegexes.h"
#include "Log.h"
#include "Parser.h"
#include "ParserHelpers.h"

namespace
{
const std::filesystem::path kCountriesFolder = std::filesystem::path("game") / "in_game" / "setup" / "countries";
const std::filesystem::path kCulturesFolder = std::filesystem::path("game") / "in_game" / "common" / "cultures";
const std::filesystem::path kReligionsFolder = std::filesystem::path("game") / "in_game" / "common" / "religions";

const std::string kTagPattern = R"([A-Z0-9]{3})";
const std::string kKeyPattern = R"([a-z_0-9]+)";
}  // namespace

eu5::GameDefinitions::GameDefinitions(const std::filesystem::path& eu5_directory)
{
   LoadKeys(eu5_directory / kCountriesFolder, tags_, kTagPattern);
   LoadKeys(eu5_directory / kCulturesFolder, cultures_, kKeyPattern);
   LoadKeys(eu5_directory / kReligionsFolder, religions_, kKeyPattern);

   if (!IsLoaded())
   {
      Log(LogLevel::Warning) << "Could not read EU5 definitions from " << eu5_directory.string()
                             << " - converted output cannot be validated against the game.";
      return;
   }
   Log(LogLevel::Info) << "<> EU5 defines " << tags_.size() << " country tags, " << cultures_.size() << " cultures and "
                       << religions_.size() << " religions.";
}

void eu5::GameDefinitions::LoadKeys(const std::filesystem::path& folder,
    std::set<std::string>& target,
    const std::string& pattern)
{
   if (!std::filesystem::exists(folder))
   {
      return;
   }
   for (const auto& entry: std::filesystem::directory_iterator(folder))
   {
      if (!entry.is_regular_file() || entry.path().extension() != ".txt")
      {
         continue;
      }
      commonItems::parser parser;
      parser.registerRegex(pattern, [&target](const std::string& key, std::istream& input_stream) {
         target.insert(key);
         commonItems::ignoreItem(key, input_stream);
      });
      parser.registerRegex(commonItems::catchallRegex, commonItems::ignoreItem);
      parser.parseFile(entry.path());
      parser.clearRegisteredKeywords();
   }
}
