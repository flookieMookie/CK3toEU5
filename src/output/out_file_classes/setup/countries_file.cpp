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
#include "src/ck3_world/titles/title.hpp"
#include "src/eu5_world/eu5_country.hpp"
#include "src/eu5_world/eu5_ruler_traits.hpp"

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

// The templates - laws, estate privileges, parliament - of the country that held a converted
// country's capital in EU5's 1337, where it was governed the same way and its religion is of the same
// group. A Catholic kingdom in France takes up France's; a Muslim one there takes up nobody's.
std::vector<std::string> TemplatesFor(const eu5::Country& country,
    const std::string& government,
    const eu5::VanillaCountry* capital_owner,
    const eu5::CountrySetup& setup)
{
   // The templates' laws come with the advances of EU5's own start, technology level 3; a country
   // behind that would start with laws it hasn't the advances for.
   constexpr int kTemplateTechnologyLevel = 3;
   if (capital_owner == nullptr || !country.GetReligion().has_value() ||
       country.GetTechnologyLevel() < kTemplateTechnologyLevel ||
       setup.GovernmentTypeOf(capital_owner->block) != government)
   {
      return {};
   }
   const auto owner_religion = setup.ReligionOf(capital_owner->tag);
   const auto group = setup.GroupOf(*country.GetReligion());
   if (!owner_religion.has_value() || !group.has_value() || setup.GroupOf(*owner_religion) != group)
   {
      return {};
   }
   std::vector<std::string> templates;
   for (const auto& name: setup.GovernmentTemplatesOf(capital_owner->block))
   {
      if (!name.ends_with("_not_present"))
      {
         templates.push_back(setup.LandlockedVariantOf(name));
      }
   }
   return templates;
}

void WriteGovernment(std::ostringstream& output,
    const eu5::Country& country,
    const std::optional<std::string>& religion_group,
    const date& conversion_date,
    const bool from_template)
{
   const auto government = GovernmentFor(country.GetSourceRealm()->GetGovernment());
   output << "\t\t\tgovernment = {\n";
   output << "\t\t\t\ttype = " << government << "\n";
   const auto& holder = country.GetSourceRealm()->GetHolder();
   const std::set<std::string> no_laws;
   const auto& laws = holder && holder->GetCharacterRealm() ? holder->GetCharacterRealm()->GetLaws() : no_laws;
   const auto heir_selection = out::HeirSelectionFor(government, laws);
   output << "\t\t\t\their_selection = " << heir_selection << "\n";
   output << "\t\t\t\tlaws = { marriage_law = " << out::MarriageLawFor(government, religion_group)
          << " heir_religion_law = " << out::HeirReligionLawFor(heir_selection) << " }\n";
   if (country.HasRuler())
   {
      output << "\t\t\t\truler = " << country.GetRulerId() << "\n";
      // When the reign began, as EU5's own countries say: it decides how many ruler traits EU5 allows.
      if (const auto& title = country.GetSourceRealm()->GetPrimaryTitle(); title)
      {
         if (const auto reign_start = eu5::ReignStart(title->GetLastHolderChangeDate(), conversion_date))
         {
            output << "\t\t\t\truler_term = { character = " << country.GetRulerId() << " start_date = " << *reign_start
                   << " }\n";
         }
      }
   }
   // Only an heir who was converted can be named; otherwise EU5 picks one as it would anyway.
   if (!country.GetHeirId().empty())
   {
      output << "\t\t\t\their = " << country.GetHeirId() << "\n";
   }
   // How decentralised the realm was is CK3's to say; a template has the rest of what EU5 asks for.
   output << "\t\t\t\tcentralization_vs_decentralization = " << out::CentralizationFor(government, laws) << "\n";
   if (!from_template)
   {
      // EU5 wants every country placed on its society axes and complains for each one that is not.
      // CK3 has no equivalent for most of them, so only the two its government type genuinely
      // speaks to are leaned; the rest sit neutral rather than inventing a position.
      output << "\t\t\t\tparliament = { parliament_type = " << ParliamentFor(government) << " }\n";
      output << "\t\t\t\ttraditionalist_vs_innovative = " << (government == "tribe" ? -40 : -20) << "\n";
      for (const auto& axis: kNeutralSocietyAxes)
      {
         output << "\t\t\t\t" << axis << " = 0\n";
      }
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

void WriteCountry(std::ostringstream& output,
    const eu5::Country& country,
    const std::vector<std::string>& templates,
    const std::optional<std::string>& religion_group,
    const std::string& discoveries,
    const date& conversion_date)
{
   output << "\n\t\t" << country.GetTag() << " = { # " << country.GetSourceRealm()->GetRealmName() << "\n";
   // Templates first, so what the conversion says about the country overrides them.
   for (const auto& name: templates)
   {
      output << "\t\t\tinclude = \"" << name << "\"\n";
   }
   output << "\t\t\tcountry_rank = " << country.GetRank() << "\n";
   output << "\t\t\tstarting_technology_level = " << country.GetTechnologyLevel() << "\n";
   if (const auto school = out::ReligiousSchoolFor(country.GetReligion()))
   {
      output << "\t\t\treligious_school = " << *school << "\n";
   }
   if (const auto& language = country.GetCourtLanguage())
   {
      output << "\t\t\tcourt_language = " << *language << "\n";
   }
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
   WriteGovernment(output, country, religion_group, conversion_date, !templates.empty());
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

// The rest of EU5's countries - those whose land the conversion took, and those vanilla already
// starts without land - written as EU5 writes a country that doesn't exist yet, so everything that
// names them still finds them. A tag a converted country took over is that country now.
int WriteCountriesNotPresent(std::ostringstream& output,
    const eu5::EU5World& eu5_world,
    const eu5::VanillaCountries& vanilla_countries,
    const eu5::MapAreas& map_areas)
{
   std::set<std::string> converted_tags;
   for (const auto& country: eu5_world.GetCountries())
   {
      if (ShouldWrite(*country))
      {
         converted_tags.insert(country->GetTag());
      }
   }
   const auto not_present = vanilla_countries.GetNotPresent(eu5_world.GetConvertedLocations(), converted_tags);
   for (const auto* vanilla: not_present)
   {
      output << "\n" << out::NotPresentBlock(vanilla->block, map_areas);
   }
   return static_cast<int>(not_present.size());
}

// Where a key = { ... } block opening at position open ends, comments skipped.
std::size_t ClosingBrace(const std::string& text, std::size_t open)
{
   int depth = 0;
   for (auto position = open; position < text.size(); ++position)
   {
      if (text[position] == '#')
      {
         position = text.find('\n', position);
         if (position == std::string::npos)
         {
            return std::string::npos;
         }
      }
      else if (text[position] == '{')
      {
         ++depth;
      }
      else if (text[position] == '}' && --depth == 0)
      {
         return position;
      }
   }
   return std::string::npos;
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

std::optional<std::string> out::ReligiousSchoolFor(const std::optional<std::string>& religion)
{
   static const std::map<std::string, std::string> kSchools = {{"sunni", "maturidi_school"},
       {"shia", "ismaili_school"},
       {"ibadi", "ibadi_school"}};
   if (!religion.has_value())
   {
      return std::nullopt;
   }
   const auto school = kSchools.find(*religion);
   return school == kSchools.end() ? std::nullopt : std::optional(school->second);
}

std::string out::HeirSelectionFor(const std::string& government, const std::set<std::string>& ck3_laws)
{
   // What EU5's own countries of each kind use.
   if (government == "tribe")
   {
      return "tribal_oldest_male";
   }
   if (government == "republic")
   {
      return "oligarchic_elective";
   }
   if (government == "theocracy")
   {
      return "theocratic_elective";
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

std::string out::MarriageLawFor(const std::string& government, const std::optional<std::string>& religion_group)
{
   if (government == "theocracy")
   {
      return "celibacy";
   }
   if (religion_group == "muslim")
   {
      return "muslim_marriage";
   }
   return "monogamous_marriage";
}

std::string out::HeirReligionLawFor(const std::string& heir_selection)
{
   // A theocracy's heir is chosen from its clergy, which EU5 marks as the special succession.
   return heir_selection == "theocratic_elective" ? "heir_special_succession" : "heir_same_religion";
}

std::string out::NotPresentBlock(const std::string& block, const eu5::MapAreas& map_areas)
{
   // EU5 wants even a country that doesn't exist to have a capital it knows the way to. Vanilla
   // leaves most to be found from the land they hold, so the first place of what it held stands in.
   static const std::regex kCapital(R"(\bcapital\s*=)");
   static const std::regex kFirstPlace(R"(\b(own_[a-z_]*|add_pops_from_locations)\s*=\s*\{\s*([a-z][A-Za-z0-9_']*))");
   static const std::regex kComment("#[^\n]*");
   const auto code = std::regex_replace(block, kComment, "");
   std::string seat;
   if (std::smatch first_place; !std::regex_search(code, kCapital) && std::regex_search(code, first_place, kFirstPlace))
   {
      seat = "\t\tcapital = " + first_place[2].str() + "\n";
      if (const auto region = map_areas.RegionOf(first_place[2].str()))
      {
         seat += "\t\tdiscovered_regions = { " + *region + " }\n";
      }
   }

   // The land it held, the pops it gathered from others' land, and the rulers of its history, none of
   // whom are in the converted world.
   static const std::set<std::string> kRemoved = {"own_control_core",
       "own_control_integrated",
       "own_control_conquered",
       "own_control_colony",
       "own_core",
       "own_conquered",
       "own_integrated",
       "own_colony",
       "control_core",
       "control",
       "our_cores_conquered_by_others",
       "add_pops_from_locations",
       "ruler_term"};
   static const std::regex kKey(R"(\b([a-z_]+)\s*=\s*\{)");
   static const std::regex kPerson(R"(\b(ruler|heir|consort|regent|active_regent)\s*=\s*(?!random\b)[A-Za-z0-9_]+)");

   std::string result = block;
   std::size_t from = 0;
   std::smatch match;
   while (std::regex_search(result.cbegin() + static_cast<std::ptrdiff_t>(from), result.cend(), match, kKey))
   {
      const auto start = from + static_cast<std::size_t>(match.position(0));
      const auto line_start = result.rfind('\n', start);
      const auto commented = result.find('#', line_start == std::string::npos ? 0 : line_start) < start;
      if (commented || !kRemoved.contains(match[1].str()))
      {
         from = start + static_cast<std::size_t>(match.length(0));
         continue;
      }
      const auto close = ClosingBrace(result, start + static_cast<std::size_t>(match.length(0)) - 1);
      if (close == std::string::npos)
      {
         break;
      }
      result.erase(start, close + 1 - start);
      from = start;
   }
   result = std::regex_replace(result, kPerson, "");

   // Drop the lines the removals emptied, keeping the block's own layout otherwise.
   std::istringstream lines(result);
   std::ostringstream output;
   std::string line;
   bool opened = false;
   while (std::getline(lines, line))
   {
      if (line.find_first_not_of(" \t\r") != std::string::npos)
      {
         output << line << "\n";
         if (!opened)
         {
            output << seat;
            opened = true;
         }
      }
   }
   return output.str();
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

   // Who held each location in EU5's 1337, whose exploration and government a converted country
   // takes on.
   std::map<std::string, const eu5::VanillaCountry*> vanilla_owner_of_location;
   for (const auto& vanilla: vanilla_countries_.GetCountries())
   {
      for (const auto& location: vanilla.locations)
      {
         vanilla_owner_of_location.emplace(location, &vanilla);
      }
   }

   int templated = 0;
   for (const auto& country: eu5_world_.GetCountries())
   {
      if (ShouldWrite(*country))
      {
         const eu5::VanillaCountry* capital_owner = nullptr;
         if (const auto& capital = country->GetCapitalLocation(); capital.has_value())
         {
            if (const auto owner = vanilla_owner_of_location.find(*capital); owner != vanilla_owner_of_location.end())
            {
               capital_owner = owner->second;
            }
         }
         const auto templates = TemplatesFor(*country,
             GovernmentFor(country->GetSourceRealm()->GetGovernment()),
             capital_owner,
             vanilla_countries_.GetSetup());
         templated += templates.empty() ? 0 : 1;
         const auto& setup = vanilla_countries_.GetSetup();
         WriteCountry(output,
             *country,
             templates,
             country->GetReligion().has_value() ? setup.GroupOf(*country->GetReligion()) : std::nullopt,
             WriteDiscoveries(country->GetLocations(),
                 capital_owner == nullptr ? std::string() : capital_owner->block,
                 map_areas_),
             eu5_world_.GetConversionDate());
      }
   }

   const auto preserved = WriteUntouchedVanillaCountries(output, eu5_world_, vanilla_countries_);
   const auto not_present = WriteCountriesNotPresent(output, eu5_world_, vanilla_countries_, map_areas_);
   Log(LogLevel::Info) << "\t<> " << templated
                       << " converted countries took up the government of who held their capital, " << preserved
                       << " vanilla countries were kept on land the conversion did not reach, and " << not_present
                       << " others start without land.";

   output << "\t}\n";
   output << "}\n";

   UseFileWriter().CreateEmptyAndWrite(folder_path / GetName(), output.str());
}

}  // namespace out
