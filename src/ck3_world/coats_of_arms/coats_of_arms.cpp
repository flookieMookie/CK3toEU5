#include "coats_of_arms.hpp"

#include <regex>
#include <string>

#include "CommonRegexes.h"
#include "Parser.h"
#include "ParserHelpers.h"

namespace
{
const std::regex kTexture(R"re((?:texture|pattern)\s*=\s*"([^"]+)")re");
}  // namespace

ck3::CoatsOfArms::CoatsOfArms(std::istream& input_stream)
{
   commonItems::parser parser;
   parser.registerKeyword("coat_of_arms_manager_database", [this](std::istream& database_stream) {
      ParseDatabase(database_stream);
   });
   parser.registerRegex(commonItems::catchallRegex, commonItems::ignoreItem);
   parser.parseStream(input_stream);
}

void ck3::CoatsOfArms::ParseDatabase(std::istream& input_stream)
{
   commonItems::parser database_parser;
   database_parser.registerRegex(R"(\d+)", [this](const std::string& id, std::istream& coat_stream) {
      CoatOfArms coat_of_arms;
      const auto item = commonItems::stringOfItem(coat_stream).getString();
      // The item comes back with its "= " in front, which would write "TAG = = {" into EU5's file.
      const auto opening_brace = item.find('{');
      if (opening_brace == std::string::npos)
      {
         return;
      }
      coat_of_arms.definition = item.substr(opening_brace);
      for (auto match = std::sregex_iterator(coat_of_arms.definition.begin(), coat_of_arms.definition.end(), kTexture);
          match != std::sregex_iterator();
          ++match)
      {
         coat_of_arms.textures.insert((*match)[1].str());
      }
      coats_of_arms_.insert_or_assign(std::stoll(id), std::move(coat_of_arms));
   });
   database_parser.registerRegex(commonItems::catchallRegex, commonItems::ignoreItem);
   database_parser.parseStream(input_stream);
}
