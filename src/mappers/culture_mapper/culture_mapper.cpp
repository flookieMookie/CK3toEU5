#include "culture_mapper.hpp"

#include <fstream>
#include <string>
#include <vector>

#include "CommonRegexes.h"
#include "Log.h"
#include "Parser.h"
#include "ParserHelpers.h"

mappers::CultureMapper::CultureMapper(std::istream& input_stream)
{
   ParseMappings(input_stream);
}

mappers::CultureMapper::CultureMapper(const std::filesystem::path& file_path)
{
   std::ifstream file_stream(file_path);
   if (!file_stream.is_open())
   {
      // Every CK3 culture EU5 shares a name with still converts; only the renamed ones are lost.
      Log(LogLevel::Warning) << "Could not open " << file_path.string() << " - CK3 cultures EU5 names differently "
                             << "will be generated instead.";
      return;
   }
   ParseMappings(file_stream);
   Log(LogLevel::Info) << "<> Loaded " << ck3_to_eu5_.size() << " culture mappings.";
}

void mappers::CultureMapper::ParseMappings(std::istream& input_stream)
{
   commonItems::parser parser;
   parser.registerKeyword("link", [this](std::istream& input_stream) {
      std::string eu5;
      std::vector<std::string> ck3;
      commonItems::parser link_parser;
      link_parser.registerKeyword("eu5", [&eu5](std::istream& input_stream) {
         eu5 = commonItems::getString(input_stream);
      });
      link_parser.registerKeyword("ck3", [&ck3](std::istream& input_stream) {
         ck3.emplace_back(commonItems::getString(input_stream));
      });
      link_parser.registerRegex(commonItems::catchallRegex, commonItems::ignoreItem);
      link_parser.parseStream(input_stream);
      if (eu5.empty())
      {
         return;
      }
      for (const auto& culture: ck3)
      {
         ck3_to_eu5_.emplace(culture, eu5);
      }
   });
   parser.registerRegex(commonItems::catchallRegex, commonItems::ignoreItem);
   parser.parseStream(input_stream);
}

std::optional<std::string> mappers::CultureMapper::GetEU5Culture(const std::string& ck3_culture) const
{
   const auto mapping = ck3_to_eu5_.find(ck3_culture);
   if (mapping == ck3_to_eu5_.end())
   {
      return std::nullopt;
   }
   return mapping->second;
}
