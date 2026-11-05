#include "dynasties.hpp"

#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <utility>

#include "CommonRegexes.h"
#include "Log.h"
#include "Parser.h"
#include "ParserHelpers.h"
#include "dynasty.hpp"
#include "house.hpp"
#include "src/ck3_world/characters/characters.hpp"


ck3::Dynasties::Dynasties(std::istream& input_stream)
{
   ParseDynasties(input_stream);
}

void ck3::Dynasties::ParseDynasties(std::istream& input_stream)
{
   registerKeyword("dynasty_house", [this](const std::string&, std::istream& input_stream) {
      ParseHouses(input_stream);
   });
   registerKeyword("dynasties", [this](const std::string&, std::istream& input_stream) {
      ParseOnlyDynasties(input_stream);
   });

   registerRegex(commonItems::catchallRegex, commonItems::ignoreItem);
   parseStream(input_stream);
   clearRegisteredKeywords();
}

int ck3::Dynasties::NameHousesFromDefinitions(std::istream& definitions)
{
   std::map<std::string, std::shared_ptr<House>> houses_by_key;
   for (const auto& [id, house]: houses_)
   {
      if (house->GetName().empty() && !house->GetKey().empty())
      {
         houses_by_key.emplace(house->GetKey(), house);
      }
   }
   int named = 0;
   commonItems::parser parser;
   parser.registerRegex(commonItems::catchallRegex, [&](const std::string& key, std::istream& input_stream) {
      std::string name;
      std::string prefix;
      commonItems::parser definition;
      definition.registerKeyword("name", [&name](const std::string&, std::istream& input_stream) {
         name = commonItems::singleString(input_stream).getString();
      });
      definition.registerKeyword("prefix", [&prefix](const std::string&, std::istream& input_stream) {
         prefix = commonItems::singleString(input_stream).getString();
      });
      definition.registerRegex(commonItems::catchallRegex, commonItems::ignoreItem);
      definition.parseStream(input_stream);
      if (const auto house = houses_by_key.find(key); house != houses_by_key.end() && !name.empty())
      {
         house->second->FillFromDefinition(name, prefix);
         houses_by_key.erase(house);
         ++named;
      }
   });
   parser.parseStream(definitions);
   return named;
}

void ck3::Dynasties::ParseOnlyDynasties(std::istream& input_stream)
{
   commonItems::parser dynasties_parser;
   dynasties_parser.registerRegex(R"(\d+)", [this](const std::string& savegame_dynasty_id, std::istream& input_stream) {
      const auto dynasty_blob_as_string = commonItems::stringOfItem(input_stream).getString();
      if (dynasty_blob_as_string.contains('{'))  // check to filter our inactive dynasties
      {
         auto dynasty_stream = std::stringstream(dynasty_blob_as_string);
         const std::shared_ptr<Dynasty> new_dynasty =
             std::make_shared<Dynasty>(dynasty_stream, std::stoll(savegame_dynasty_id));
         dynasties_.insert(std::make_pair(new_dynasty->GetSavegameDynastyID(), new_dynasty));
      }
   });
   dynasties_parser.registerRegex(commonItems::catchallRegex, commonItems::ignoreItem);
   dynasties_parser.parseStream(input_stream);
   dynasties_parser.clearRegisteredKeywords();
}

void ck3::Dynasties::ParseHouses(std::istream& input_stream)
{
   commonItems::parser house_parser;
   house_parser.registerRegex(R"(\d+)", [this](const std::string& house_id, std::istream& input_stream) {
      const auto house_blob_as_string = commonItems::stringOfItem(input_stream).getString();
      if (house_blob_as_string.contains('{'))  // check to filter our inactive houses
      {
         auto house_stream = std::stringstream(house_blob_as_string);
         const std::shared_ptr<House> new_house = std::make_shared<House>(house_stream, std::stoll(house_id));
         houses_.insert(std::make_pair(new_house->GetID(), new_house));
      }
   });
   house_parser.registerRegex(commonItems::catchallRegex, commonItems::ignoreItem);
   house_parser.parseStream(input_stream);
   house_parser.clearRegisteredKeywords();
}

void ck3::Dynasties::LinkCharacters(const Characters& characters)
{
   for (auto& house: houses_)
   {
      house.second->LinkHouseHead(characters.GetAllCharacters());
   }
   for (auto& dynasty: dynasties_)
   {
      dynasty.second->LinkCharacters(characters.GetAllCharacters());
   }
   Log(LogLevel::Debug) << "Dynasties characters linked.";
}

void ck3::Dynasties::LinkDynasties()
{
   for (auto& house: houses_)
   {
      house.second->LinkDynasty(dynasties_);
   }
   Log(LogLevel::Debug) << "Dynasties houses linked.";
}
