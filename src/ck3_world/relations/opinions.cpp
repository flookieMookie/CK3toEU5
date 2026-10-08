#include "opinions.hpp"

#include <sstream>
#include <string>

#include "CommonRegexes.h"
#include "Parser.h"
#include "ParserHelpers.h"

ck3::Opinions::Opinions(std::istream& input_stream)
{
   commonItems::parser parser;
   parser.registerKeyword("active_opinions", [this](std::istream& opinions_stream) {
      // A list of unnamed blocks, one per character with an opinion of another.
      for (const auto& blob: commonItems::blobList(opinions_stream).getBlobs())
      {
         long long owner = 0;
         long long target = 0;
         std::set<std::string> relations;
         commonItems::parser opinion_parser;
         opinion_parser.registerKeyword("owner", [&owner](std::istream& value_stream) {
            owner = commonItems::getLlong(value_stream);
         });
         opinion_parser.registerKeyword("target", [&target](std::istream& value_stream) {
            target = commonItems::getLlong(value_stream);
         });
         opinion_parser.registerKeyword("scripted_relations", [&relations](std::istream& relations_stream) {
            commonItems::parser relations_parser;
            relations_parser.registerRegex(commonItems::catchallRegex,
                [&relations](const std::string& relation, std::istream& relation_stream) {
                   relations.insert(relation);
                   commonItems::ignoreItem(relation, relation_stream);
                });
            relations_parser.parseStream(relations_stream);
         });
         opinion_parser.registerRegex(commonItems::catchallRegex, commonItems::ignoreItem);
         auto blob_stream = std::stringstream(blob);
         opinion_parser.parseStream(blob_stream);

         if (owner != 0 && target != 0 && owner != target && !relations.empty())
         {
            scripted_relations_[{owner, target}].insert(relations.begin(), relations.end());
         }
      }
   });
   parser.registerRegex(commonItems::catchallRegex, commonItems::ignoreItem);
   parser.parseStream(input_stream);
}
