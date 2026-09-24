#include "vanilla_start_file.hpp"

#include <external/commonItems/Log.h>

#include <algorithm>
#include <fstream>
#include <iterator>
#include <regex>
#include <sstream>
#include <string>
#include <utility>

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
}  // namespace

std::string out::TransformEntries(const std::string& contents,
    const int entry_depth,
    const std::function<std::optional<std::string>(const std::string& entry)>& transform)
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
      std::string entry = line + "\n";
      int entry_depth_change = DepthChange(line);
      while (entry_depth_change > 0 && std::getline(input, line))
      {
         entry += line + "\n";
         entry_depth_change += DepthChange(line);
      }
      if (const auto replacement = transform(entry); replacement.has_value())
      {
         output << *replacement;
      }
   }
   return output.str();
}

std::set<std::string> out::TagsNamedIn(const std::string& text)
{
   std::set<std::string> tags;
   std::istringstream lines(text);
   std::string line;
   while (std::getline(lines, line))
   {
      const auto code = WithoutComment(line);
      for (auto match = std::sregex_iterator(code.begin(), code.end(), kTag); match != std::sregex_iterator(); ++match)
      {
         tags.insert(match->str());
      }
   }
   return tags;
}

std::string out::KeepEntriesAbout(const std::string& contents, const int entry_depth, const std::set<std::string>& allowed_tags)
{
   return TransformEntries(contents, entry_depth, [&allowed_tags](const std::string& entry) -> std::optional<std::string> {
      if (std::ranges::all_of(TagsNamedIn(entry), [&allowed_tags](const std::string& tag) {
             return allowed_tags.contains(tag);
          }))
      {
         return entry;
      }
      return std::nullopt;
   });
}

std::string out::WithoutMissingCharacters(const std::string& text, const std::set<std::string>& characters)
{
   static const std::regex kCharacter(R"(\bcharacter\s*=\s*([a-z0-9_]+))");
   static const std::regex kArtist(R"(\bartist\s*=\s*([a-z0-9_]+)\s*)");
   std::istringstream lines(text);
   std::ostringstream kept;
   std::string line;
   while (std::getline(lines, line))
   {
      const auto code = WithoutComment(line);
      if (std::smatch character; std::regex_search(code, character, kCharacter) && !characters.contains(character[1].str()))
      {
         continue;
      }
      if (std::smatch artist; std::regex_search(code, artist, kArtist) && !characters.contains(artist[1].str()))
      {
         line = artist.prefix().str() + artist.suffix().str() + line.substr(code.size());
      }
      kept << line << "\n";
   }
   return kept.str();
}

std::set<std::string> out::KeptVanillaCharacters(const eu5::VanillaCharacters& vanilla_characters,
    const std::vector<const eu5::VanillaCountry*>& kept_countries)
{
   std::set<std::string> characters;
   for (const auto& character: vanilla_characters.KeptFor(kept_countries))
   {
      characters.insert(character.id);
   }
   return characters;
}

std::optional<std::string> out::FitBuilding(const std::string& entry, const BuildingOwnership& ownership)
{
   static const std::regex kOwner(R"(\btag\s*=\s*([A-Z0-9]{3})\b)");
   static const std::regex kLocation(R"(\blocation\s*=\s*([A-Za-z0-9_']+))");
   const auto code = WithoutComment(entry);
   std::smatch owner;
   std::smatch location;
   if (!std::regex_search(code, owner, kOwner) || !std::regex_search(code, location, kLocation))
   {
      return entry;
   }
   const auto tag = owner[1].str();
   const auto place = location[1].str();

   const auto vanilla_owner = ownership.vanilla_owners.find(place);
   const bool foreign_owned = vanilla_owner == ownership.vanilla_owners.end() || vanilla_owner->second != tag;
   if (foreign_owned)
   {
      if (ownership.kept_tags.contains(tag) && vanilla_owner != ownership.vanilla_owners.end() &&
          ownership.kept_tags.contains(vanilla_owner->second))
      {
         return entry;
      }
      return std::nullopt;
   }

   const auto current_owner = ownership.current_owners.find(place);
   if (current_owner == ownership.current_owners.end())
   {
      return std::nullopt;
   }
   if (entry.contains("seat_of_cardinal") && !ownership.kept_tags.contains(current_owner->second))
   {
      const auto religion = ownership.religions.find(current_owner->second);
      if (religion == ownership.religions.end() || religion->second != "catholic")
      {
         return std::nullopt;
      }
   }
   // Only the owner changes; the level, the location and any comment stay as EU5 wrote them.
   return std::regex_replace(entry, kOwner, "tag = " + current_owner->second, std::regex_constants::format_first_only);
}

namespace out
{

VanillaStartFile::VanillaStartFile(const std::string& name,
    FileWriter& file_writer,
    const eu5::EU5World& eu5_world,
    const eu5::VanillaCountries& vanilla_countries,
    std::filesystem::path eu5_directory,
    const int entry_depth,
    std::function<std::string()> converted_entries):
    OutputFile(name, file_writer),
    eu5_world_(eu5_world),
    vanilla_countries_(vanilla_countries),
    eu5_directory_(std::move(eu5_directory)),
    entry_depth_(entry_depth),
    converted_entries_(std::move(converted_entries))
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
   auto kept = KeepEntriesAbout(contents, entry_depth_, kept_tags);
   if (converted_entries_)
   {
      const auto closing = kept.rfind('}');
      kept.insert(closing == std::string::npos ? kept.size() : closing, converted_entries_());
   }
   Log(LogLevel::Info) << "\t<> Kept " << GetName() << " only where it concerns the " << kept_tags.size()
                       << " vanilla countries the conversion keeps.";
   UseFileWriter().CreateEmptyAndWrite(folder_path / GetName(), "\xEF\xBB\xBF" + kept);
}

}  // namespace out

namespace out
{

VanillaLocationsFile::VanillaLocationsFile(const std::string& name,
    FileWriter& file_writer,
    const eu5::EU5World& eu5_world,
    const eu5::VanillaCountries& vanilla_countries,
    const eu5::VanillaCharacters& vanilla_characters,
    std::filesystem::path eu5_directory):
    OutputFile(name, file_writer),
    eu5_world_(eu5_world),
    vanilla_countries_(vanilla_countries),
    vanilla_characters_(vanilla_characters),
    eu5_directory_(std::move(eu5_directory))
{
}

void VanillaLocationsFile::Create(const std::filesystem::path& folder_path)
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

   BuildingOwnership ownership;
   for (const auto& country: vanilla_countries_.GetCountries())
   {
      for (const auto& location: country.locations)
      {
         ownership.vanilla_owners.emplace(location, country.tag);
      }
   }
   const auto kept_countries = vanilla_countries_.GetUntouched(eu5_world_.GetConvertedLocations());
   for (const auto* country: kept_countries)
   {
      ownership.kept_tags.insert(country->tag);
      for (const auto& location: country->locations)
      {
         ownership.current_owners.emplace(location, country->tag);
      }
   }
   for (const auto& country: eu5_world_.GetCountries())
   {
      if (!country->IsWritten())
      {
         continue;
      }
      for (const auto& location: country->GetLocations())
      {
         ownership.current_owners.insert_or_assign(location, country->GetTag());
      }
      if (country->GetReligion().has_value())
      {
         ownership.religions.emplace(country->GetTag(), *country->GetReligion());
      }
   }

   int kept = 0;
   int dropped = 0;
   const auto buildings_fitted = TransformEntries(contents, 1, [&](const std::string& entry) {
      auto result = FitBuilding(entry, ownership);
      ++(result.has_value() ? kept : dropped);
      return result;
   });
   const auto fitted =
       WithoutMissingCharacters(buildings_fitted, KeptVanillaCharacters(vanilla_characters_, kept_countries));
   Log(LogLevel::Info) << "\t<> " << GetName() << ": kept " << kept << " entries fitted to the converted world, "
                       << "dropped " << dropped << ".";
   UseFileWriter().CreateEmptyAndWrite(folder_path / GetName(), "\xEF\xBB\xBF" + fitted);
}

}  // namespace out
