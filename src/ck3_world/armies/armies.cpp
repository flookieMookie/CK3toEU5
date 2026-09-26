#include "armies.hpp"

#include <string>

#include "CommonRegexes.h"
#include "Parser.h"
#include "ParserHelpers.h"

ck3::Armies::Armies(std::istream& input_stream)
{
   commonItems::parser parser;
   parser.registerKeyword("regiments", [this](std::istream& regiments_stream) {
      commonItems::parser regiments_parser;
      regiments_parser.registerRegex(commonItems::integerRegex, [this](const std::string&, std::istream& regiment_stream) {
         std::string type;
         long long owner = 0;
         int size = 0;
         commonItems::parser regiment_parser;
         regiment_parser.registerKeyword("type", [&type](std::istream& value_stream) {
            type = commonItems::getString(value_stream);
         });
         regiment_parser.registerKeyword("owner", [&owner](std::istream& value_stream) {
            owner = commonItems::getLlong(value_stream);
         });
         regiment_parser.registerKeyword("size", [&size](std::istream& value_stream) {
            size = commonItems::getInt(value_stream);
         });
         regiment_parser.registerRegex(commonItems::catchallRegex, commonItems::ignoreItem);
         regiment_parser.parseStream(regiment_stream);
         if (!type.empty() && owner != 0 && size > 0)
         {
            men_at_arms_[owner] += size;
         }
      });
      regiments_parser.registerRegex(commonItems::catchallRegex, commonItems::ignoreItem);
      regiments_parser.parseStream(regiments_stream);
   });
   parser.registerRegex(commonItems::catchallRegex, commonItems::ignoreItem);
   parser.parseStream(input_stream);
}
