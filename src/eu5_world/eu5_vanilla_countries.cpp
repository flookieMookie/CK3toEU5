#include "eu5_vanilla_countries.hpp"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <regex>
#include <sstream>
#include <string>

#include "Log.h"

namespace
{
const std::filesystem::path kCountriesFile =
    std::filesystem::path("game") / "main_menu" / "setup" / "start" / "10_countries.txt";

// Country blocks sit two levels down, inside countries = { countries = { ... } }, and the file is
// far easier to carry over verbatim than to reproduce: each block holds includes, government,
// ruler terms and centuries of ruler history that the converter has no way to regenerate.
const std::regex kCountryStart(R"(^\s*([A-Z0-9]{3})\s*=\s*\{)");

// Every bare token inside an ownership list is a location name; the own_ lists are what it owns.
void CollectLocations(const std::string& block, eu5::VanillaCountry& country)
{
   static const std::regex kOwnershipBlock(R"((own|control|our)[a-z_]*\s*=\s*\{([^{}]*)\})");
   static const std::regex kComment("#[^\n]*");
   const auto code = std::regex_replace(block, kComment, "");
   for (auto match = std::sregex_iterator(code.begin(), code.end(), kOwnershipBlock); match != std::sregex_iterator();
       ++match)
   {
      const auto owns = (*match)[1].str() == "own";
      std::istringstream tokens((*match)[2].str());
      std::string token;
      while (tokens >> token)
      {
         if (!token.empty() && (std::islower(static_cast<unsigned char>(token.front())) != 0))
         {
            country.locations.insert(token);
            if (owns)
            {
               country.owned.insert(token);
            }
         }
      }
   }
}
}  // namespace

eu5::VanillaCountries::VanillaCountries(const std::filesystem::path& eu5_directory): setup_(eu5_directory)
{
   const auto countries_file = eu5_directory / kCountriesFile;
   if (!std::filesystem::exists(countries_file))
   {
      Log(LogLevel::Warning) << "EU5 countries not found at " << countries_file.string()
                             << " - land outside CK3's map will be left unowned.";
      return;
   }
   Parse(countries_file);
   Log(LogLevel::Info) << "<> Read " << countries_.size() << " vanilla EU5 countries.";
}

void eu5::VanillaCountries::Parse(const std::filesystem::path& file_path)
{
   std::ifstream file(file_path);
   std::string line;
   while (std::getline(file, line))
   {
      std::smatch match;
      if (!std::regex_search(line, match, kCountryStart))
      {
         continue;
      }

      // Take the block verbatim, brace matching from this line onwards.
      VanillaCountry country;
      country.tag = match[1].str();
      country.block = line + "\n";

      int depth = 0;
      for (const char character: line)
      {
         depth += character == '{' ? 1 : (character == '}' ? -1 : 0);
      }
      while (depth > 0 && std::getline(file, line))
      {
         country.block += line + "\n";
         for (const char character: line)
         {
            depth += character == '{' ? 1 : (character == '}' ? -1 : 0);
         }
      }

      CollectLocations(country.block, country);
      countries_.emplace_back(std::move(country));
   }
}

std::vector<const eu5::VanillaCountry*> eu5::VanillaCountries::GetUntouched(
    const std::set<std::string>& converted_locations) const
{
   std::vector<const VanillaCountry*> untouched;
   for (const auto& country: countries_)
   {
      const bool overlaps = std::ranges::any_of(country.locations, [&converted_locations](const auto& location) {
         return converted_locations.contains(location);
      });
      if (!country.locations.empty() && !overlaps)
      {
         untouched.push_back(&country);
      }
   }
   return untouched;
}

std::vector<const eu5::VanillaCountry*> eu5::VanillaCountries::GetNotPresent(
    const std::set<std::string>& converted_locations,
    const std::set<std::string>& converted_tags) const
{
   auto taken = converted_tags;
   for (const auto* country: GetUntouched(converted_locations))
   {
      taken.insert(country->tag);
   }
   std::vector<const VanillaCountry*> not_present;
   for (const auto& country: countries_)
   {
      if (taken.insert(country.tag).second)
      {
         not_present.push_back(&country);
      }
   }
   return not_present;
}
