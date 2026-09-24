#include "international_organizations_file.hpp"

#include <external/commonItems/Log.h>

#include <algorithm>
#include <fstream>
#include <iterator>
#include <regex>
#include <sstream>
#include <string>
#include <utility>

#include "vanilla_start_file.hpp"

namespace
{
const std::regex kMembers(R"(members\s*=\s*\{[^}]*\})");
const std::regex kLeader(R"(\bleader\s*=\s*([A-Z0-9]{3})\b)");
const std::regex kReligion(R"(\breligion\s*=\s*religion:([a-z0-9_]+))");
const std::regex kType(R"(\btype\s*=\s*([a-z0-9_]+))");
const std::regex kCharacter(R"(\bcharacter\s*=\s*([a-z0-9_]+))");

// The religion a religious organisation serves, where its entry says or its type implies it.
// Political ones - the HRE, the Ilkhanate - have none.
std::optional<std::string> ReligionOf(const std::string& organization)
{
   if (std::smatch religion; std::regex_search(organization, religion, kReligion))
   {
      return religion[1].str();
   }
   if (std::smatch type; std::regex_search(organization, type, kType))
   {
      if (type[1].str() == "catholic_church")
      {
         return "catholic";
      }
      if (type[1].str() == "shinto")
      {
         return "shinto";
      }
   }
   return std::nullopt;
}

// Removes each line naming a character the mod doesn't define: ruler terms of popes, patriarchs and
// emperors from EU5's own history.
std::string WithoutMissingCharacters(const std::string& organization, const std::set<std::string>& characters)
{
   std::istringstream lines(organization);
   std::ostringstream kept;
   std::string line;
   while (std::getline(lines, line))
   {
      std::smatch character;
      if (std::regex_search(line, character, kCharacter) && !characters.contains(character[1].str()))
      {
         continue;
      }
      kept << line << "\n";
   }
   return kept.str();
}
}  // namespace

std::optional<std::string> out::FitOrganization(const std::string& organization,
    const OrganizationMembership& membership)
{
   const auto religion = ReligionOf(organization);
   const auto qualifies = [&membership, &religion](const std::string& tag) {
      if (membership.kept_tags.contains(tag))
      {
         return true;
      }
      const auto converted = membership.converted_religions.find(tag);
      return religion.has_value() && converted != membership.converted_religions.end() && converted->second == *religion;
   };

   std::smatch members_match;
   if (!std::regex_search(organization, members_match, kMembers))
   {
      return std::nullopt;
   }
   std::string members;
   for (const auto& tag: TagsNamedIn(members_match.str()))
   {
      if (qualifies(tag))
      {
         members += tag + " ";
      }
   }
   if (members.empty())
   {
      return std::nullopt;
   }
   if (std::smatch leader; std::regex_search(organization, leader, kLeader) && !qualifies(leader[1].str()))
   {
      return std::nullopt;
   }

   auto fitted = members_match.prefix().str() + "members = { " + members + "}" + members_match.suffix().str();
   fitted = WithoutMissingCharacters(fitted, membership.characters);

   // Anything else it names - a Tatar overlord, a tax collector - has to be in good standing too.
   const auto outside_members = std::regex_replace(fitted, kMembers, "");
   if (!std::ranges::all_of(TagsNamedIn(outside_members), qualifies))
   {
      return std::nullopt;
   }
   return fitted;
}

namespace out
{

InternationalOrganizationsFile::InternationalOrganizationsFile(const std::string& name,
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

void InternationalOrganizationsFile::Create(const std::filesystem::path& folder_path)
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

   OrganizationMembership membership;
   for (const auto* country: vanilla_countries_.GetUntouched(eu5_world_.GetConvertedLocations()))
   {
      membership.kept_tags.insert(country->tag);
   }
   for (const auto& country: eu5_world_.GetCountries())
   {
      if (country->IsWritten() && country->GetReligion().has_value())
      {
         membership.converted_religions.emplace(country->GetTag(), *country->GetReligion());
      }
   }
   // Converted characters never appear in EU5's own history, so only the vanilla ones carried over
   // for kept countries can be named there.
   for (const auto& character: vanilla_characters_.GetCharacters())
   {
      if (membership.kept_tags.contains(character.tag))
      {
         membership.characters.insert(character.id);
      }
   }

   int kept = 0;
   int dropped = 0;
   const auto fitted = TransformEntries(contents, 1, [&](const std::string& organization) {
      auto result = FitOrganization(organization, membership);
      ++(result.has_value() ? kept : dropped);
      return result;
   });

   Log(LogLevel::Info) << "\t<> Kept " << kept << " of EU5's international organisations, fitted to the converted "
                       << "world; dropped " << dropped << ".";
   UseFileWriter().CreateEmptyAndWrite(folder_path / GetName(), "\xEF\xBB\xBF" + fitted);
}

}  // namespace out
