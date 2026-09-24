#include "eu5_vanilla_characters.hpp"

#include <fstream>
#include <istream>
#include <map>
#include <regex>
#include <set>
#include <sstream>
#include <string>

#include "Log.h"

namespace
{
const std::filesystem::path kCharactersFile =
    std::filesystem::path("game") / "main_menu" / "setup" / "start" / "05_characters.txt";

// Characters sit one level down, inside character_db = { ... }. Their own contents nest further -
// first_name = { ... } - so a block only starts a character at that first depth.
const std::regex kBlockStart(R"(^\s*([a-z0-9_]+)\s*=\s*\{)");
const std::regex kTag(R"(\btag\s*=\s*([A-Z0-9]{3})\b)");
const std::regex kValue(R"(=\s*([a-z][a-z0-9_]*))");
const std::regex kFamily(R"(^\s*(father|mother|spouse)\s*=\s*([a-z0-9_]+))");

std::string WithoutComment(const std::string& line)
{
   const auto comment = line.find('#');
   return comment == std::string::npos ? line : line.substr(0, comment);
}

// A character of a replaced country, moved to the kept country naming it, with its family links to
// characters who are gone removed.
std::string Adopt(const std::string& block, const std::string& tag, const std::set<std::string>& known)
{
   std::istringstream lines(block);
   std::ostringstream adopted;
   std::string line;
   while (std::getline(lines, line))
   {
      const auto code = WithoutComment(line);
      if (std::smatch family; std::regex_search(code, family, kFamily) && !known.contains(family[2].str()))
      {
         continue;
      }
      adopted << std::regex_replace(line, kTag, "tag = " + tag) << "\n";
   }
   return adopted.str();
}

int DepthChange(const std::string& line)
{
   int change = 0;
   for (const char character: line)
   {
      if (character == '#')
      {
         break;  // braces in comments don't count
      }
      change += character == '{' ? 1 : (character == '}' ? -1 : 0);
   }
   return change;
}
}  // namespace

eu5::VanillaCharacters::VanillaCharacters(const std::filesystem::path& eu5_directory)
{
   std::ifstream file(eu5_directory / kCharactersFile);
   if (!file.is_open())
   {
      Log(LogLevel::Warning) << "EU5 characters not found - vanilla countries the conversion keeps will have no ruler.";
      return;
   }
   Parse(file);
   Log(LogLevel::Info) << "<> Read " << characters_.size() << " vanilla EU5 characters.";
}

eu5::VanillaCharacters::VanillaCharacters(std::istream& input_stream)
{
   Parse(input_stream);
}

void eu5::VanillaCharacters::Parse(std::istream& input_stream)
{
   int depth = 0;
   std::string line;
   while (std::getline(input_stream, line))
   {
      if (!line.empty() && line.front() == '\xEF')
      {
         line.erase(0, 3);  // byte order mark
      }
      std::smatch match;
      if (depth != 1 || !std::regex_search(line, match, kBlockStart))
      {
         depth += DepthChange(line);
         continue;
      }

      VanillaCharacter character;
      character.id = match[1].str();
      character.block = line + "\n";
      int block_depth = DepthChange(line);
      while (block_depth > 0 && std::getline(input_stream, line))
      {
         character.block += line + "\n";
         block_depth += DepthChange(line);
      }
      if (std::smatch tag; std::regex_search(character.block, tag, kTag))
      {
         character.tag = tag[1].str();
      }
      characters_.emplace_back(std::move(character));
   }
}

std::vector<eu5::VanillaCharacter> eu5::VanillaCharacters::KeptFor(
    const std::vector<const VanillaCountry*>& kept_countries) const
{
   std::set<std::string> kept_tags;
   for (const auto* country: kept_countries)
   {
      kept_tags.insert(country->tag);
   }
   std::vector<VanillaCharacter> kept;
   std::set<std::string> known;
   std::map<std::string, const VanillaCharacter*> by_id;
   for (const auto& character: characters_)
   {
      by_id.emplace(character.id, &character);
      if (kept_tags.contains(character.tag))
      {
         kept.push_back(character);
         known.insert(character.id);
      }
   }

   std::vector<std::pair<const VanillaCharacter*, std::string>> borrowed;
   for (const auto* country: kept_countries)
   {
      static const std::regex kComment("#[^\n]*");
      const auto code = std::regex_replace(country->block, kComment, "");
      for (auto value = std::sregex_iterator(code.begin(), code.end(), kValue); value != std::sregex_iterator(); ++value)
      {
         const auto character = by_id.find((*value)[1].str());
         if (character != by_id.end() && !known.contains(character->first))
         {
            known.insert(character->first);
            borrowed.emplace_back(character->second, country->tag);
         }
      }
   }
   for (const auto& [character, tag]: borrowed)
   {
      kept.push_back({.id = character->id, .tag = tag, .block = Adopt(character->block, tag, known)});
   }
   return kept;
}
