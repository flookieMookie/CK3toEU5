#include "eu5_vanilla_characters.hpp"

#include <fstream>
#include <istream>
#include <regex>
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
