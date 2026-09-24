#include "house.hpp"

#include <iostream>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>

#include "CommonRegexes.h"
#include "Log.h"
#include "ParserHelpers.h"
#include "dynasty.hpp"
#include "src/ck3_world/characters/character.hpp"
#include "src/ck3_world/id_pointer_pair.hpp"

ck3::House::House(std::istream& input_stream, long long house_id): house_id_(house_id)
{
   ParseHouse(input_stream);
}

void ck3::House::ParseHouse(std::istream& input_stream)
{
   registerKeyword("key", [this](std::istream& input_stream) {
      key_ = commonItems::getString(input_stream);
   });
   registerKeyword("name", [this](const std::string&, std::istream& input_stream) {
      name_ = commonItems::singleString(input_stream).getString();
   });
   registerKeyword("localized_name", [this](const std::string&, std::istream& input_stream) {
      localized_name_ = commonItems::singleString(input_stream).getString();
   });
   registerKeyword("prefix", [this](const std::string&, std::istream& input_stream) {
      prefix_ = commonItems::singleString(input_stream).getString();
   });
   registerKeyword("dynasty", [this](const std::string&, std::istream& input_stream) {
      dynasty_ = IdPointerPair<Dynasty>(commonItems::singleLlong(input_stream).getLlong());
   });
   registerKeyword("head_of_house", [this](const std::string&, std::istream& input_stream) {
      house_head_ = IdPointerPair<Character>(commonItems::singleLlong(input_stream).getLlong());
   });
   registerRegex(commonItems::catchallRegex, commonItems::ignoreItem);
   parseStream(input_stream);
   clearRegisteredKeywords();
}


void ck3::House::LinkHouseHead(const std::map<long long, std::shared_ptr<Character>>& characters_map)
{
   if (house_head_.has_value())
   {
      if (characters_map.contains(house_head_->GetID()))
      {
         house_head_->SetPointer(characters_map.at(house_head_->GetID()));
      }
      else
      {
         // As with dynasties, a pruned house head is expected on a long campaign.
         Log(LogLevel::Debug) << "House " << house_id_ << " has house head " << house_head_->GetID()
                              << " who is no longer in the save, ignoring them.";
         house_head_.reset();
      }
   }
}

void ck3::House::LinkDynasty(const std::map<long long, std::shared_ptr<Dynasty>>& dynasties_map)
{
   if (dynasties_map.contains(dynasty_.GetID()))
   {
      dynasty_.SetPointer(dynasties_map.at(dynasty_.GetID()));
   }
   else
   {
      // The link is left empty rather than aborting the conversion.
      Log(LogLevel::Warning) << "House " << house_id_ << " belongs to dynasty " << dynasty_.GetID()
                             << " which has no definition, ignoring it.";
   }
}
