#include "country_definitions_file.hpp"

#include <external/commonItems/Log.h>

#include <cstdint>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <utility>

#include "src/ck3_world/realms/realm.hpp"
#include "src/ck3_world/titles/title.hpp"
#include "src/eu5_world/eu5_country.hpp"

namespace
{
// EU5 accepts a literal rgb colour, so a converted country does not need a named map colour.
// Derived from the tag so a given realm keeps the same colour between runs.
std::string ColourFor(const std::string& tag)
{
   std::uint32_t hash = 2166136261U;
   for (const char character: tag)
   {
      hash ^= static_cast<std::uint8_t>(character);
      hash *= 16777619U;
   }
   // Kept mid range so countries stay legible against the map.
   const auto red = 60 + (hash >> 16U & 0xFFU) % 170;
   const auto green = 60 + (hash >> 8U & 0xFFU) % 170;
   const auto blue = 60 + (hash & 0xFFU) % 170;
   return "rgb { " + std::to_string(red) + " " + std::to_string(green) + " " + std::to_string(blue) + " }";
}
}  // namespace

namespace out
{

CountryDefinitionsFile::CountryDefinitionsFile(const std::string& name,
    FileWriter& file_writer,
    const eu5::EU5World& eu5_world,
    const eu5::VanillaCountries& vanilla_countries):
    OutputFile(name, file_writer),
    eu5_world_(eu5_world),
    vanilla_countries_(vanilla_countries)
{
}

void CountryDefinitionsFile::WriteRedefinedTags(const std::filesystem::path& folder_path)
{
   const auto& setup = vanilla_countries_.GetSetup();
   std::map<std::string, eu5::Redefinition> redefinitions;
   std::set<std::string> converted_tags;
   int reused = 0;
   for (const auto& country: eu5_world_.GetCountries())
   {
      if (!country->IsWritten())
      {
         continue;
      }
      converted_tags.insert(country->GetTag());
      if (country->NeedsDefinition())
      {
         continue;
      }
      // A tag EU5 keeps for history is a real country again.
      auto& redefinition = redefinitions[country->GetTag()];
      redefinition.historic = false;
      if (country->GetCulture().has_value() && country->GetCulture() != setup.CultureOf(country->GetTag()))
      {
         redefinition.culture = country->GetCulture();
      }
      if (country->GetReligion().has_value() && country->GetReligion() != setup.ReligionOf(country->GetTag()))
      {
         redefinition.religion = country->GetReligion();
      }
      reused += redefinition.culture.has_value() || redefinition.religion.has_value() ? 1 : 0;
   }
   // The rest of EU5's 1337 countries don't exist in the converted world; EU5 asks for those to be
   // marked as kept only for history.
   const auto not_present = vanilla_countries_.GetNotPresent(eu5_world_.GetConvertedLocations(), converted_tags);
   for (const auto* vanilla: not_present)
   {
      redefinitions[vanilla->tag].historic = true;
   }

   int files = 0;
   for (const auto& [file_name, text]: setup.GetDefinitionFiles())
   {
      const auto rewritten = eu5::Redefine(text, redefinitions);
      if (rewritten != text)
      {
         // Under EU5's own name, so it stands in for that file.
         UseFileWriter().CreateEmptyAndWrite(folder_path / file_name, rewritten);
         ++files;
      }
   }
   Log(LogLevel::Info) << "\t<> Gave " << reused << " of EU5's tags the culture and religion of the converted "
                       << "countries using them and marked " << not_present.size() << " as history only, in " << files
                       << " of its definition files.";
}

void CountryDefinitionsFile::Create(const std::filesystem::path& folder_path)
{
   Log(LogLevel::Info) << "\tCreating " << GetName();
   WriteRedefinedTags(folder_path);

   std::ostringstream output;
   // EU5 reads its database files as UTF-8 with a byte order mark, and warns about any without.
   output << "\xEF\xBB\xBF# Country definitions for converted tags EU5 does not ship.\n";

   int written = 0;
   for (const auto& country: eu5_world_.GetCountries())
   {
      if (!country->NeedsDefinition() || country->GetLocations().empty())
      {
         continue;
      }
      // Without a culture and a religion EU5 will not accept the definition, so skip rather than
      // write something it will reject.
      if (!country->GetCulture().has_value() || !country->GetReligion().has_value())
      {
         continue;
      }

      output << "\n"
             << country->GetTag() << " = { # " << country->GetSourceRealm()->GetRealmName() << " ("
             << country->GetSourceRealm()->GetPrimaryTitle()->GetKey() << ")\n";
      output << "\tcolor = " << ColourFor(country->GetTag()) << "\n";
      output << "\tculture_definition = " << *country->GetCulture() << "\n";
      output << "\treligion_definition = " << *country->GetReligion() << "\n";
      output << "}\n";
      ++written;
   }

   Log(LogLevel::Info) << "\t<> Wrote " << written << " generated country definitions.";
   UseFileWriter().CreateEmptyAndWrite(folder_path / GetName(), output.str());
}

}  // namespace out
