#include "religions.hpp"

#include <iostream>
#include <memory>
#include <string>
#include <utility>

#include "CommonRegexes.h"
#include "Log.h"
#include "Parser.h"
#include "ParserHelpers.h"
#include "faith.hpp"
#include "religion.hpp"
#include "src/ck3_world/titles/title.hpp"
#include "src/ck3_world/titles/titles.hpp"

ck3::Religions::Religions(std::istream& input_stream)
{
   ParseReligions(input_stream);
}

void ck3::Religions::ParseReligions(std::istream& input_stream)
{
   registerKeyword("religions", [this](const std::string&, std::istream& input_stream) {
      ParseOnlyReligions(input_stream);
   });
   registerKeyword("faiths", [this](const std::string&, std::istream& input_stream) {
      ParseFaiths(input_stream);
   });
   registerRegex(commonItems::catchallRegex, commonItems::ignoreItem);
   parseStream(input_stream);
   clearRegisteredKeywords();
}

void ck3::Religions::ParseFaithDatabase(std::istream& input_stream)
{
   commonItems::parser database_parser;
   database_parser.registerKeyword("database", [this](const std::string&, std::istream& input_stream) {
      ParseFaiths(input_stream);
   });
   database_parser.registerRegex(commonItems::catchallRegex, commonItems::ignoreItem);
   database_parser.parseStream(input_stream);
   database_parser.clearRegisteredKeywords();
}

void ck3::Religions::ParseRites(std::istream& input_stream)
{
   const auto parse_rite = [this](const std::string& rite_id, std::istream& input_stream) {
      std::string tag;
      std::string name;
      std::string source;
      long long faith_id = -1;
      commonItems::parser rite_parser;
      rite_parser.registerKeyword("rite_type", [&tag](const std::string&, std::istream& input_stream) {
         tag = commonItems::singleString(input_stream).getString();
      });
      rite_parser.registerKeyword("faith", [&faith_id](const std::string&, std::istream& input_stream) {
         faith_id = commonItems::singleLlong(input_stream).getLlong();
      });
      rite_parser.registerKeyword("source", [&source](const std::string&, std::istream& input_stream) {
         source = commonItems::singleString(input_stream).getString();
      });
      rite_parser.registerKeyword("data", [&tag, &name](const std::string&, std::istream& input_stream) {
         commonItems::parser data_parser;
         data_parser.registerKeyword("tag", [&tag](const std::string&, std::istream& input_stream) {
            const auto data_tag = commonItems::singleString(input_stream).getString();
            if (tag.empty())
            {
               tag = data_tag;
            }
         });
         data_parser.registerKeyword("name", [&name](const std::string&, std::istream& input_stream) {
            name = commonItems::singleString(input_stream).getString();
         });
         data_parser.registerRegex(commonItems::catchallRegex, commonItems::ignoreItem);
         data_parser.parseStream(input_stream);
      });
      rite_parser.registerRegex(commonItems::catchallRegex, commonItems::ignoreItem);
      rite_parser.parseStream(input_stream);
      // CK3's own rites have their names in its localisation; only a campaign's own needs its name kept.
      const auto id = std::stoll(rite_id);
      rites_.emplace(id, std::make_shared<Faith>(id, tag, source == "historical" ? "" : name, faith_id));
   };
   commonItems::parser database_parser;
   database_parser.registerKeyword("database", [&parse_rite](const std::string&, std::istream& input_stream) {
      commonItems::parser rites_parser;
      rites_parser.registerRegex(R"(\d+)", parse_rite);
      rites_parser.registerRegex(commonItems::catchallRegex, commonItems::ignoreItem);
      rites_parser.parseStream(input_stream);
   });
   database_parser.registerRegex(commonItems::catchallRegex, commonItems::ignoreItem);
   database_parser.parseStream(input_stream);
   database_parser.clearRegisteredKeywords();
}

void ck3::Religions::ParseOnlyReligions(std::istream& input_stream)
{
   commonItems::parser religions_parser;
   religions_parser.registerRegex(R"(\d+)", [this](const std::string& religion_id, std::istream& input_stream) {
      const std::shared_ptr<Religion> new_religion = std::make_shared<Religion>(input_stream, std::stoll(religion_id));
      religions_.insert(std::make_pair(new_religion->GetID(), new_religion));
   });
   religions_parser.registerRegex(commonItems::catchallRegex, commonItems::ignoreItem);
   religions_parser.parseStream(input_stream);
   religions_parser.clearRegisteredKeywords();
}

void ck3::Religions::ParseFaiths(std::istream& input_stream)
{
   commonItems::parser faiths_parser;
   faiths_parser.registerRegex(R"(\d+)", [this](const std::string& faith_id, std::istream& input_stream) {
      const std::shared_ptr<Faith> new_faith = std::make_shared<Faith>(input_stream, std::stoll(faith_id));
      faiths_.insert(std::make_pair(new_faith->GetID(), new_faith));
   });
   faiths_parser.registerRegex(commonItems::catchallRegex, commonItems::ignoreItem);
   faiths_parser.parseStream(input_stream);
   faiths_parser.clearRegisteredKeywords();
}

void ck3::Religions::LinkTitles(const Titles& titles)
{
   // Titles are keyed by name, and religious heads reference them by ID, so build a cache.
   std::map<long long, std::shared_ptr<Title>> id_title_map;
   for (const auto& title: titles.GetTitles())
   {
      id_title_map.insert(std::pair(title.second->GetID(), title.second));
   }

   for (const auto& faith: faiths_)
   {
      faith.second->LinkReligiousHead(id_title_map);
   }
   Log(LogLevel::Debug) << "Religious heads linked.";
}
void ck3::Religions::LinkReligions()
{
   // A rite's religion is its faith's, so faiths come first.
   for (const auto& faith: faiths_)
   {
      faith.second->LinkReligion(religions_);
   }
   for (const auto& rite: rites_)
   {
      rite.second->LinkParentFaith(faiths_, rite.second);
      rite.second->LinkReligion(religions_);
   }
   for (const auto& religion: religions_)
   {
      religion.second->LinkFaiths(faiths_);
   }
   Log(LogLevel::Debug) << "Religions linked.";
}