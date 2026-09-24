#include "vanilla_start_file.hpp"

#include <external/commonItems/Log.h>

#include <fstream>
#include <iterator>
#include <regex>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace
{
const std::regex kTag(R"(\b[A-Z][A-Z0-9]{2}\b)");

std::string WithoutComment(const std::string& line)
{
   const auto comment = line.find('#');
   return comment == std::string::npos ? line : line.substr(0, comment);
}

int DepthChange(const std::string& line)
{
   int change = 0;
   for (const char character: WithoutComment(line))
   {
      change += character == '{' ? 1 : (character == '}' ? -1 : 0);
   }
   return change;
}

bool OnlyAbout(const std::vector<std::string>& lines, const std::set<std::string>& allowed_tags)
{
   for (const auto& line: lines)
   {
      const auto code = WithoutComment(line);
      for (auto match = std::sregex_iterator(code.begin(), code.end(), kTag); match != std::sregex_iterator(); ++match)
      {
         if (!allowed_tags.contains(match->str()))
         {
            return false;
         }
      }
   }
   return true;
}
}  // namespace

std::string out::KeepEntriesAbout(const std::string& contents, const int entry_depth, const std::set<std::string>& allowed_tags)
{
   std::istringstream input(contents);
   std::ostringstream output;
   std::string line;
   int depth = 0;
   while (std::getline(input, line))
   {
      const auto code = WithoutComment(line);
      const bool starts_entry = depth == entry_depth && code.find_first_not_of(" \t\r") != std::string::npos &&
                                code.find_first_not_of(" \t\r}") != std::string::npos;
      if (!starts_entry)
      {
         output << line << "\n";
         depth += DepthChange(line);
         continue;
      }

      // Gather the whole entry: this line, and any lines its braces open.
      std::vector<std::string> entry{line};
      int entry_depth_change = DepthChange(line);
      while (entry_depth_change > 0 && std::getline(input, line))
      {
         entry.push_back(line);
         entry_depth_change += DepthChange(line);
      }
      if (OnlyAbout(entry, allowed_tags))
      {
         for (const auto& entry_line: entry)
         {
            output << entry_line << "\n";
         }
      }
   }
   return output.str();
}

namespace out
{

VanillaStartFile::VanillaStartFile(const std::string& name,
    FileWriter& file_writer,
    const eu5::EU5World& eu5_world,
    const eu5::VanillaCountries& vanilla_countries,
    std::filesystem::path eu5_directory,
    const int entry_depth):
    OutputFile(name, file_writer),
    eu5_world_(eu5_world),
    vanilla_countries_(vanilla_countries),
    eu5_directory_(std::move(eu5_directory)),
    entry_depth_(entry_depth)
{
}

void VanillaStartFile::Create(const std::filesystem::path& folder_path)
{
   Log(LogLevel::Info) << "\tCreating " << GetName();

   std::ifstream vanilla(eu5_directory_ / "game" / "main_menu" / "setup" / "start" / GetName());
   if (!vanilla.is_open())
   {
      Log(LogLevel::Warning) << "\t<> EU5's " << GetName() << " not found - skipping, vanilla will apply.";
      return;
   }
   std::string contents((std::istreambuf_iterator<char>(vanilla)), std::istreambuf_iterator<char>());
   if (contents.starts_with("\xEF\xBB\xBF"))
   {
      contents.erase(0, 3);
   }

   std::set<std::string> kept_tags;
   for (const auto* country: vanilla_countries_.GetUntouched(eu5_world_.GetConvertedLocations()))
   {
      kept_tags.insert(country->tag);
   }
   const auto kept = KeepEntriesAbout(contents, entry_depth_, kept_tags);
   Log(LogLevel::Info) << "\t<> Kept " << GetName() << " only where it concerns the " << kept_tags.size()
                       << " vanilla countries the conversion keeps.";
   UseFileWriter().CreateEmptyAndWrite(folder_path / GetName(), "\xEF\xBB\xBF" + kept);
}

}  // namespace out
