#include "relations.hpp"

#include <algorithm>
#include <sstream>
#include <string>

#include "CommonRegexes.h"
#include "Parser.h"
#include "ParserHelpers.h"

ck3::Relations::Relations(std::istream& input_stream)
{
   commonItems::parser parser;
   parser.registerKeyword("active_relations", [this](std::istream& relations_stream) {
      ParseActiveRelations(relations_stream);
   });
   parser.registerRegex(commonItems::catchallRegex, commonItems::ignoreItem);
   parser.parseStream(input_stream);
}

void ck3::Relations::ParseActiveRelations(std::istream& input_stream)
{
   // A list of unnamed blocks, one per pair of characters with anything between them.
   for (const auto& blob: commonItems::blobList(input_stream).getBlobs())
   {
      long long first = 0;
      long long second = 0;
      bool allied = false;
      commonItems::parser relation_parser;
      relation_parser.registerKeyword("first", [&first](std::istream& value_stream) {
         first = commonItems::getLlong(value_stream);
      });
      relation_parser.registerKeyword("second", [&second](std::istream& value_stream) {
         second = commonItems::getLlong(value_stream);
      });
      relation_parser.registerKeyword("alliances", [&allied](std::istream& value_stream) {
         allied = true;
         commonItems::ignoreItem("alliances", value_stream);
      });
      relation_parser.registerRegex(commonItems::catchallRegex, commonItems::ignoreItem);
      auto blob_stream = std::stringstream(blob);
      relation_parser.parseStream(blob_stream);

      if (allied && first != 0 && second != 0 && first != second)
      {
         alliances_.emplace(std::min(first, second), std::max(first, second));
      }
   }
}
