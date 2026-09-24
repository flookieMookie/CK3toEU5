#include "eu5_map_areas.hpp"

#include <fstream>
#include <iterator>
#include <vector>

#include "Log.h"

eu5::MapAreas::MapAreas(const std::filesystem::path& eu5_directory)
{
   std::ifstream file(eu5_directory / "game" / "in_game" / "map_data" / "definitions.txt");
   if (!file.is_open())
   {
      Log(LogLevel::Warning) << "EU5 map definitions not found - countries at war start without raised levies.";
      return;
   }
   Parse(file);
   Log(LogLevel::Info) << "<> Placed " << area_of_location_.size() << " EU5 locations in their areas.";
}

eu5::MapAreas::MapAreas(std::istream& input_stream)
{
   Parse(input_stream);
}

void eu5::MapAreas::Parse(std::istream& input_stream)
{
   std::string contents((std::istreambuf_iterator<char>(input_stream)), std::istreambuf_iterator<char>());
   if (contents.starts_with("\xEF\xBB\xBF"))
   {
      contents.erase(0, 3);
   }

   // The blocks open around each location: the nearest one whose name ends in _area is its area. A
   // word followed by { names a block; any other word is a location.
   std::vector<std::string> open_blocks;
   std::string previous_word;
   std::string token;
   bool in_comment = false;
   const auto place_previous_word = [&]() {
      if (previous_word.empty())
      {
         return;
      }
      for (auto block = open_blocks.rbegin(); block != open_blocks.rend(); ++block)
      {
         if (block->ends_with("_area"))
         {
            area_of_location_.emplace(previous_word, *block);
            break;
         }
      }
      previous_word.clear();
   };
   const auto finish_token = [&]() {
      if (!token.empty())
      {
         place_previous_word();
         previous_word = token;
         token.clear();
      }
   };

   for (const char character: contents)
   {
      if (in_comment)
      {
         in_comment = character != '\n';
         continue;
      }
      switch (character)
      {
         case '#':
            finish_token();
            in_comment = true;
            break;
         case '{':
            finish_token();
            open_blocks.push_back(previous_word);
            previous_word.clear();
            break;
         case '}':
            finish_token();
            place_previous_word();
            if (!open_blocks.empty())
            {
               open_blocks.pop_back();
            }
            break;
         case '=':
         case ' ':
         case '\t':
         case '\r':
         case '\n':
            finish_token();
            break;
         default:
            token += character;
      }
   }
}

std::optional<std::string> eu5::MapAreas::AreaOf(const std::string& location) const
{
   if (const auto area = area_of_location_.find(location); area != area_of_location_.end())
   {
      return area->second;
   }
   return std::nullopt;
}
