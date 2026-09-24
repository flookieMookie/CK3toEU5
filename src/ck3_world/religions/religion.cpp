#include "religion.hpp"

#include <iostream>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>

#include "CommonRegexes.h"
#include "Log.h"
#include "ParserHelpers.h"
#include "faith.hpp"

ck3::Religion::Religion(std::istream& input_stream, long long religion_id): religion_id_(religion_id)
{
   ParseReligion(input_stream);
}

void ck3::Religion::ParseReligion(std::istream& input_stream)
{
   registerKeyword("tag", [this](const std::string&, std::istream& input_stream) {
      tag_ = commonItems::singleString(input_stream).getString();
   });
   registerKeyword("family", [this](const std::string&, std::istream& input_stream) {
      family_ = commonItems::singleString(input_stream).getString();
   });
   registerKeyword("faiths", [this](const std::string&, std::istream& input_stream) {
      for (auto faith: commonItems::llongList(input_stream).getLlongs())
      {
         faiths_.emplace_back(faith);
      }
   });
   registerKeyword("religion_type", [this](const std::string&, std::istream& input_stream) {
      religion_type_ = commonItems::singleString(input_stream).getString();
   });
   registerRegex(commonItems::catchallRegex, commonItems::ignoreItem);
   parseStream(input_stream);
   clearRegisteredKeywords();
}


void ck3::Religion::LinkFaiths(const std::map<long long, std::shared_ptr<Faith>>& faith_map)
{
   // A faith missing from the save is dropped from the religion rather than aborting the conversion.
   std::erase_if(faiths_, [this, &faith_map](const auto& faith) {
      if (faith_map.contains(faith.GetID()))
      {
         return false;
      }
      Log(LogLevel::Warning) << "Religion " << religion_id_ << " lists faith " << faith.GetID()
                             << " which has no definition, ignoring it.";
      return true;
   });
   for (auto& faith: faiths_)
   {
      faith.SetPointer(faith_map.at(faith.GetID()));
   }
}