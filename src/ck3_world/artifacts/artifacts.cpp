#include "artifacts.hpp"

#include <sstream>
#include <string>

#include "CommonRegexes.h"
#include "Parser.h"
#include "ParserHelpers.h"
#include "StringUtils.h"
#include "src/ck3_world/wars/wars.hpp"

namespace
{
// history = { entries = { { type = created date = 1066.9.25 } { type = inherited ... } } }: the first
// entry with a date is where the artifact's story starts.
std::optional<date> ParseCreated(std::istream& input_stream)
{
   std::optional<date> created;
   commonItems::parser history_parser;
   history_parser.registerKeyword("entries", [&created](std::istream& entries_stream) {
      for (const auto& blob: commonItems::blobList(entries_stream).getBlobs())
      {
         commonItems::parser entry_parser;
         entry_parser.registerKeyword("date", [&created](std::istream& value_stream) {
            const date when(commonItems::getString(value_stream));
            if (!created.has_value() || when < *created)
            {
               created = when;
            }
         });
         entry_parser.registerRegex(commonItems::catchallRegex, commonItems::ignoreItem);
         auto blob_stream = std::stringstream(blob);
         entry_parser.parseStream(blob_stream);
      }
   });
   history_parser.registerRegex(commonItems::catchallRegex, commonItems::ignoreItem);
   history_parser.parseStream(input_stream);
   return created;
}
}  // namespace

ck3::Artifacts::Artifacts(std::istream& input_stream)
{
   commonItems::parser parser;
   parser.registerKeyword("artifacts", [this](std::istream& artifacts_stream) {
      commonItems::parser list_parser;
      list_parser.registerRegex(commonItems::integerRegex,
          [this](const std::string& id, std::istream& artifact_stream) {
             // A destroyed artifact is left behind as id=none.
             const auto item = commonItems::stringOfItem(artifact_stream).getString();
             const auto open = item.find('{');
             if (open == std::string::npos)
             {
                return;
             }
             Artifact artifact{.id = std::stoll(id)};
             commonItems::parser artifact_parser;
             artifact_parser.registerKeyword("name", [&artifact](std::istream& value_stream) {
                artifact.name = WithoutMarkup(commonItems::remQuotes(commonItems::getString(value_stream)));
             });
             artifact_parser.registerKeyword("description", [&artifact](std::istream& value_stream) {
                artifact.description = WithoutMarkup(commonItems::remQuotes(commonItems::getString(value_stream)));
             });
             artifact_parser.registerKeyword("type", [&artifact](std::istream& value_stream) {
                artifact.type = commonItems::getString(value_stream);
             });
             artifact_parser.registerKeyword("rarity", [&artifact](std::istream& value_stream) {
                artifact.rarity = commonItems::getString(value_stream);
             });
             artifact_parser.registerKeyword("owner", [&artifact](std::istream& value_stream) {
                artifact.owner = commonItems::getLlong(value_stream);
             });
             artifact_parser.registerKeyword("history", [&artifact](std::istream& history_stream) {
                artifact.created = ParseCreated(history_stream);
             });
             artifact_parser.registerRegex(commonItems::catchallRegex, commonItems::ignoreItem);
             auto item_stream = std::stringstream(item.substr(open));
             artifact_parser.parseStream(item_stream);
             if (artifact.owner != 0 && !artifact.type.empty())
             {
                artifacts_.push_back(std::move(artifact));
             }
          });
      list_parser.registerRegex(commonItems::catchallRegex, commonItems::ignoreItem);
      list_parser.parseStream(artifacts_stream);
   });
   parser.registerRegex(commonItems::catchallRegex, commonItems::ignoreItem);
   parser.parseStream(input_stream);
}
