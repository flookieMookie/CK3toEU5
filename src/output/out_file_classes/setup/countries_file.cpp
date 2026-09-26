#include "countries_file.hpp"

#include <external/commonItems/Log.h>

#include <algorithm>
#include <map>
#include <regex>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include "src/ck3_world/characters/character.hpp"
#include "src/ck3_world/realms/realm.hpp"
#include "src/eu5_world/eu5_country.hpp"

namespace
{
// Vanilla's own 10_countries.txt wraps the blocks in an outer countries = { countries = { ... } }
// and keeps the location lists to readable line lengths.
constexpr int kLocationsPerLine = 8;

// What vanilla starts in. EU5 has a single start date, so there is no other age to choose.
const std::string kCurrentAge = "age_1_traditions";

// CK3's government types onto the ones EU5 accepts in a country's government block.
std::string GovernmentFor(const std::string& ck3_government)
{
   if (ck3_government.starts_with("tribal"))
   {
      return "tribe";
   }
   if (ck3_government.starts_with("theocracy"))
   {
      return "theocracy";
   }
   if (ck3_government.starts_with("republic"))
   {
      return "republic";
   }
   // Feudal and clan both sit closest to a monarchy.
   return "monarchy";
}

std::string ParliamentFor(const std::string& government)
{
   if (government == "tribe")
   {
      return "assembly";
   }
   if (government == "republic")
   {
      return "estate_parliament";
   }
   return "council";
}

// The axes CK3 says nothing about. Writing them neutral is honest and stops EU5 complaining that
// the country has no society values scripted.
const std::vector<std::string> kNeutralSocietyAxes = {"spiritualist_vs_humanist",
    "aristocracy_vs_plutocracy",
    "serfdom_vs_free_subjects",
    "mercantilism_vs_free_trade",
    "belligerent_vs_conciliatory",
    "quality_vs_quantity",
    "offensive_vs_defensive",
    "land_vs_naval",
    "capital_economy_vs_traditional_economy",
    "individualism_vs_communalism",
    "outward_vs_inward"};

bool ShouldWrite(const eu5::Country& country)
{
   // A tag EU5 does not define needs one written alongside this file. Where that could not be
   // generated the tag would be rejected, and a rejected block takes the rest of the file with it,
   // so the country is left out entirely.
   return country.IsWritten();
}

void WriteGovernment(std::ostringstream& output, const eu5::Country& country)
{
   const auto government = GovernmentFor(country.GetSourceRealm()->GetGovernment());
   output << "\t\t\tgovernment = {\n";
   output << "\t\t\t\ttype = " << government << "\n";
   const auto& holder = country.GetSourceRealm()->GetHolder();
   if (holder && holder->GetCharacterRealm())
   {
      if (const auto heir_selection = out::HeirSelectionFor(government, holder->GetCharacterRealm()->GetLaws()))
      {
         output << "\t\t\t\their_selection = " << *heir_selection << "\n";
      }
   }
   if (country.HasRuler())
   {
      output << "\t\t\t\truler = " << country.GetRulerId() << "\n";
   }
   // Only an heir who was converted can be named; otherwise EU5 picks one as it would anyway.
   if (!country.GetHeirId().empty())
   {
      output << "\t\t\t\their = " << country.GetHeirId() << "\n";
   }
   output << "\t\t\t\tparliament = { parliament_type = " << ParliamentFor(government) << " }\n";
   // EU5 wants every country placed on its society axes and complains for each one that is not.
   // CK3 has no equivalent for most of them, so only the two its government type genuinely speaks
   // to are leaned; the rest sit neutral rather than inventing a position.
   const std::set<std::string> no_laws;
   const auto& laws = holder && holder->GetCharacterRealm() ? holder->GetCharacterRealm()->GetLaws() : no_laws;
   output << "\t\t\t\tcentralization_vs_decentralization = " << out::CentralizationFor(government, laws) << "\n";
   output << "\t\t\t\ttraditionalist_vs_innovative = " << (government == "tribe" ? -40 : -20) << "\n";
   for (const auto& axis: kNeutralSocietyAxes)
   {
      output << "\t\t\t\t" << axis << " = 0\n";
   }
   output << "\t\t\t}\n\n";
}

void WriteLocations(std::ostringstream& output, const eu5::Country& country)
{
   output << "\t\t\town_control_core = {\n";
   int on_this_line = 0;
   for (const auto& location: country.GetLocations())
   {
      if (on_this_line == 0)
      {
         output << "\t\t\t\t";
      }
      output << location << " ";
      if (++on_this_line == kLocationsPerLine)
      {
         output << "\n";
         on_this_line = 0;
      }
   }
   if (on_this_line != 0)
   {
      output << "\n";
   }
   output << "\t\t\t}\n";
}

void WriteCountry(std::ostringstream& output, const eu5::Country& country, const std::string& discoveries)
{
   output << "\n\t\t" << country.GetTag() << " = { # " << country.GetSourceRealm()->GetRealmName() << "\n";
   output << "\t\t\tcountry_rank = " << country.GetRank() << "\n";
   output << "\t\t\tstarting_technology_level = " << country.GetTechnologyLevel() << "\n";
   output << discoveries;
   // The ruler's treasury, which EU5 keeps in the same place its own start data does.
   if (country.HasRuler())
   {
      if (const auto gold = eu5::StartingGoldFor(country.GetSourceRealm()->GetHolder()->GetGold()); gold != 0)
      {
         output << "\t\t\tcurrency_data = { gold = " << gold << " }\n";
      }
   }
   output << "\n";
   WriteGovernment(output, country);
   WriteLocations(output, country);
   output << "\t\t}\n";
}

// Everything CK3 does not cover - the Americas, Oceania, much of Siberia - would otherwise be left
// with no owner at all, since this file replaces EU5's wholesale. Vanilla countries whose land the
// conversion never touched are carried over exactly as EU5 wrote them.
int WriteUntouchedVanillaCountries(std::ostringstream& output,
    const eu5::EU5World& eu5_world,
    const eu5::VanillaCountries& vanilla_countries)
{
   int preserved = 0;
   for (const auto* vanilla: vanilla_countries.GetUntouched(eu5_world.GetConvertedLocations()))
   {
      output << "\n" << vanilla->block;
      ++preserved;
   }
   return preserved;
}
}  // namespace

std::string out::WriteDiscoveries(const std::vector<std::string>& locations,
    const std::string& capital_owner_block,
    const eu5::MapAreas& map_areas)
{
   static const std::regex kExploration(R"re(include\s*=\s*"(expl_[a-z_]+)")re");
   static const std::regex kComment("#[^\n]*");
   std::ostringstream output;
   const auto owner_block = std::regex_replace(capital_owner_block, kComment, "");
   for (auto include = std::sregex_iterator(owner_block.begin(), owner_block.end(), kExploration);
       include != std::sregex_iterator();
       ++include)
   {
      output << "\t\t\tinclude = \"" << (*include)[1].str() << "\"\n";
   }
   std::set<std::string> regions;
   for (const auto& location: locations)
   {
      if (const auto region = map_areas.RegionOf(location))
      {
         regions.insert(*region);
      }
   }
   if (!regions.empty())
   {
      output << "\t\t\tdiscovered_regions = {";
      for (const auto& region: regions)
      {
         output << " " << region;
      }
      output << " }\n";
   }
   return output.str();
}

int out::CentralizationFor(const std::string& government, const std::set<std::string>& ck3_laws)
{
   for (int level = 0; level <= 3; ++level)
   {
      if (ck3_laws.contains("crown_authority_" + std::to_string(level)))
      {
         return 60 - 20 * level;
      }
      if (ck3_laws.contains("tribal_authority_" + std::to_string(level)))
      {
         return 80 - 20 * level;
      }
   }
   return government == "tribe" ? 40 : -20;
}

std::optional<std::string> out::HeirSelectionFor(const std::string& government, const std::set<std::string>& ck3_laws)
{
   if (government != "monarchy")
   {
      return std::nullopt;
   }
   // Every CK3 partition law - confederate, high, the clans' - divides the realm among the sons.
   if (std::ranges::any_of(ck3_laws, [](const std::string& law) {
          return law.contains("partition_succession_law");
       }))
   {
      return "partition_inheritance";
   }
   if (ck3_laws.contains("male_only_law"))
   {
      return "salic_law";
   }
   // EU5 has no law preferring or requiring women; equal primogeniture is the nearest.
   if (ck3_laws.contains("equal_law") || ck3_laws.contains("female_preference_law") ||
       ck3_laws.contains("female_only_law"))
   {
      return "absolute_cognatic_primogeniture";
   }
   // Male preference, and anything a campaign or mod adds, is EU5's own default.
   return "cognatic_primogeniture";
}

namespace out
{

CountriesFile::CountriesFile(const std::string& name,
    FileWriter& file_writer,
    const eu5::EU5World& eu5_world,
    const eu5::VanillaCountries& vanilla_countries,
    const eu5::MapAreas& map_areas):
    OutputFile(name, file_writer),
    eu5_world_(eu5_world),
    vanilla_countries_(vanilla_countries),
    map_areas_(map_areas)
{
}

void CountriesFile::Create(const std::filesystem::path& folder_path)
{
   Log(LogLevel::Info) << "\tCreating " << GetName();

   std::ostringstream output;
   output << "current_age = " << kCurrentAge << "\n\n";
   output << "countries = {\n";
   output << "\tcountries = {\n";

   // Who held each location in EU5's 1337, whose exploration a converted country takes on.
   std::map<std::string, const std::string*> vanilla_block_of_location;
   for (const auto& vanilla: vanilla_countries_.GetCountries())
   {
      for (const auto& location: vanilla.locations)
      {
         vanilla_block_of_location.emplace(location, &vanilla.block);
      }
   }

   for (const auto& country: eu5_world_.GetCountries())
   {
      if (ShouldWrite(*country))
      {
         std::string capital_owner_block;
         if (const auto& capital = country->GetCapitalLocation(); capital.has_value())
         {
            if (const auto owner = vanilla_block_of_location.find(*capital); owner != vanilla_block_of_location.end())
            {
               capital_owner_block = *owner->second;
            }
         }
         WriteCountry(output, *country, WriteDiscoveries(country->GetLocations(), capital_owner_block, map_areas_));
      }
   }

   const auto preserved = WriteUntouchedVanillaCountries(output, eu5_world_, vanilla_countries_);
   Log(LogLevel::Info) << "\t<> Kept " << preserved << " vanilla countries on land the conversion did not reach.";

   output << "\t}\n";
   output << "}\n";

   UseFileWriter().CreateEmptyAndWrite(folder_path / GetName(), output.str());
}

}  // namespace out
