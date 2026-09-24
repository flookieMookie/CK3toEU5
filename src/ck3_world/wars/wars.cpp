#include "wars.hpp"

#include <regex>
#include <sstream>
#include <string>

#include "CommonRegexes.h"
#include "Parser.h"
#include "ParserHelpers.h"
#include "StringUtils.h"

namespace
{
// CK3 renders war names with its text markup in them, each code opened by \x15: a link -
// \x15ONCLICK:TITLE,3615 - or tooltip up to the next space, a style - \x15L; - and \x15! to close
// one. What is left is the name the player read.
std::string WithoutMarkup(const std::string& text)
{
   static const std::regex kLinks("\x15(ONCLICK|TOOLTIP):[^ ]* ?");
   static const std::regex kStyles("\x15[A-Za-z_]+; ?");
   static const std::regex kEnds("\x15!");
   static const std::regex kSpaces(" {2,}");
   auto plain = std::regex_replace(text, kLinks, "");
   plain = std::regex_replace(plain, kStyles, "");
   plain = std::regex_replace(plain, kEnds, "");
   plain = std::regex_replace(plain, kSpaces, " ");
   const auto first = plain.find_first_not_of(' ');
   const auto last = plain.find_last_not_of(' ');
   return first == std::string::npos ? std::string{} : plain.substr(first, last - first + 1);
}

// One side of a war: participants = { { character = 12631 ... } { ... } }.
std::vector<long long> ParseParticipants(std::istream& input_stream)
{
   std::vector<long long> characters;
   commonItems::parser side_parser;
   side_parser.registerKeyword("participants", [&characters](std::istream& participants_stream) {
      for (const auto& blob: commonItems::blobList(participants_stream).getBlobs())
      {
         long long character = 0;
         commonItems::parser participant_parser;
         participant_parser.registerKeyword("character", [&character](std::istream& value_stream) {
            character = commonItems::getLlong(value_stream);
         });
         participant_parser.registerRegex(commonItems::catchallRegex, commonItems::ignoreItem);
         auto blob_stream = std::stringstream(blob);
         participant_parser.parseStream(blob_stream);
         if (character != 0)
         {
            characters.push_back(character);
         }
      }
   });
   side_parser.registerRegex(commonItems::catchallRegex, commonItems::ignoreItem);
   side_parser.parseStream(input_stream);
   return characters;
}
}  // namespace

ck3::Wars::Wars(std::istream& input_stream)
{
   commonItems::parser parser;
   parser.registerKeyword("active_wars", [this](std::istream& wars_stream) {
      ParseActiveWars(wars_stream);
   });
   parser.registerRegex(commonItems::catchallRegex, commonItems::ignoreItem);
   parser.parseStream(input_stream);
}

void ck3::Wars::ParseActiveWars(std::istream& input_stream)
{
   commonItems::parser wars_parser;
   wars_parser.registerRegex(commonItems::integerRegex, [this](const std::string&, std::istream& war_stream) {
      // A war that has ended stays in the list as "none" until the save is next compacted.
      const auto item = commonItems::stringOfItem(war_stream).getString();
      const auto open = item.find('{');
      if (open == std::string::npos)
      {
         return;
      }

      War war;
      commonItems::parser war_parser;
      war_parser.registerKeyword("attacker", [&war](std::istream& side_stream) {
         war.attackers = ParseParticipants(side_stream);
      });
      war_parser.registerKeyword("defender", [&war](std::istream& side_stream) {
         war.defenders = ParseParticipants(side_stream);
      });
      war_parser.registerKeyword("start_date", [&war](std::istream& value_stream) {
         war.start_date = date(commonItems::getString(value_stream));
      });
      war_parser.registerKeyword("name", [&war](std::istream& value_stream) {
         war.name = WithoutMarkup(commonItems::remQuotes(commonItems::getString(value_stream)));
      });
      war_parser.registerKeyword("casus_belli", [&war](std::istream& cb_stream) {
         commonItems::parser cb_parser;
         cb_parser.registerKeyword("type", [&war](std::istream& value_stream) {
            war.casus_belli = commonItems::getString(value_stream);
         });
         cb_parser.registerKeyword("attacker", [&war](std::istream& value_stream) {
            war.attacker = commonItems::getLlong(value_stream);
         });
         cb_parser.registerKeyword("defender", [&war](std::istream& value_stream) {
            war.defender = commonItems::getLlong(value_stream);
         });
         cb_parser.registerKeyword("targeted_titles", [&war](std::istream& value_stream) {
            war.targeted_titles = commonItems::getLlongs(value_stream);
         });
         cb_parser.registerRegex(commonItems::catchallRegex, commonItems::ignoreItem);
         cb_parser.parseStream(cb_stream);
      });
      war_parser.registerRegex(commonItems::catchallRegex, commonItems::ignoreItem);
      auto war_body = std::stringstream(item.substr(open));
      war_parser.parseStream(war_body);

      if (war.attacker != 0 && war.defender != 0 && war.attacker != war.defender)
      {
         wars_.push_back(std::move(war));
      }
   });
   wars_parser.registerRegex(commonItems::catchallRegex, commonItems::ignoreItem);
   wars_parser.parseStream(input_stream);
}
