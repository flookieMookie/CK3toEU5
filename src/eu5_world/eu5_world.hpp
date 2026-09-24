#ifndef EU5_WORLD_H
#define EU5_WORLD_H

#include <Date.h>

#include <cstddef>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "eu5_country.hpp"
#include "src/ck3_world/coats_of_arms/coats_of_arms.hpp"
#include "eu5_culture_resolver.hpp"
#include "eu5_game_definitions.hpp"
#include "eu5_location_data.hpp"

namespace ck3
{
class CK3World;
class Character;
class Culture;
class Faith;
class House;
class Realm;
class Title;
class VassalContracts;
class Relations;
}

namespace mappers
{
class Mappers;
}

namespace eu5
{

// A CK3 vassal relationship, written out as an EU5 subject.
struct Dependency
{
   std::string liege_tag;
   std::string vassal_tag;
   // vassal for CK3 vassalage, tributary for CK3 tributaries.
   std::string subject_type = "vassal";
};

// A country's part in a converted war, as EU5 asks it to join: the leader as Instigator or Target,
// the rest called in by a country already in it - its liege as Subject, the leader as an ally.
struct WarParticipant
{
   std::string tag;
   std::string reason;  // Instigator, Target, Subject or Scripted
   std::string caller;  // empty for the leaders
};

// A CK3 war between rulers who both became independent countries, fought in EU5 over one location
// of the land at stake.
struct ConvertedWar
{
   std::string name_key;  // localisation key for the CK3 name, or empty if the save gave none
   std::string name;
   date start_date = date("1.1.1");
   std::string target_location;
   std::vector<WarParticipant> attackers;
   std::vector<WarParticipant> defenders;
};

// A CK3 house written as an EU5 dynasty. EU5's dynasty is the family name a character carries,
// which in CK3 is the house - Karling - rather than the wider dynasty.
struct ConvertedDynasty
{
   std::string id;
   std::shared_ptr<ck3::House> house;
   // An EU5 location, required for every dynasty: the capital of the first country it rules.
   std::string home;
};

// Turns the independent CK3 realms into EU5 countries: a tag, a capital, and the EU5 locations the
// realm holds, walked down from its counties through their baronies.
class EU5World
{
  public:
   EU5World() = default;
   EU5World(const ck3::CK3World& ck3_world,
       const mappers::Mappers& mappers,
       const GameDefinitions& game_definitions,
       const LocationData& location_data);

   [[nodiscard]] const auto& GetCountries() const { return countries_; }
   [[nodiscard]] auto GetRealmsWithoutTag() const { return realms_without_tag_; }
   [[nodiscard]] auto GetCountiesWithoutBaronies() const { return counties_without_baronies_; }
   // The CK3 save's date, so converted rulers can be aged onto EU5's start date instead of being
   // born five centuries before the campaign begins.
   [[nodiscard]] const auto& GetConversionDate() const { return conversion_date_; }
   // CK3's trait names, indexed by the IDs its characters' traits carry.
   [[nodiscard]] const auto& GetCK3TraitNames() const { return ck3_trait_names_; }
   [[nodiscard]] const auto& GetLocationReligions() const { return location_religions_; }
   [[nodiscard]] const auto& GetLocationCultures() const { return location_cultures_; }
   [[nodiscard]] const auto& GetCultureResolver() const { return culture_resolver_; }
   [[nodiscard]] const auto& GetDependencies() const { return dependencies_; }
   // Every EU5 location any converted country holds.
   [[nodiscard]] std::set<std::string> GetConvertedLocations() const;
   // Each allied pair of independent countries, as tags, once.
   [[nodiscard]] const auto& GetAlliances() const { return alliances_; }
   [[nodiscard]] const auto& GetDynasties() const { return dynasties_; }
   [[nodiscard]] const auto& GetWars() const { return wars_; }
   // Country tag to the CK3 coat of arms it flies, for the tags the conversion invents.
   [[nodiscard]] const auto& GetFlags() const { return flags_; }
   // The EU5 dynasty a converted character belongs to, or empty.
   [[nodiscard]] std::string DynastyIdOf(const ck3::Character& character) const;
   [[nodiscard]] const auto& GetDevelopmentBonuses() const { return development_bonuses_; }

   void LogReport() const;

  private:
   // CK3 tributaries rule their own land, so they are already countries; this makes them subjects.
   void AssignTributaries(const ck3::VassalContracts& contracts);
   // Alliances between CK3 rulers who both became independent countries.
   void AssignAlliances(const ck3::Relations& relations);
   // Each converted ruler's living spouse, children and heir, as characters alongside them.
   void AssignFamilies(const ck3::CK3World& ck3_world);
   // The houses of everyone converted, as EU5 dynasties.
   void AssignDynasties();
   // Arms from CK3 for the countries EU5 has no flag for.
   void AssignFlags(const ck3::CK3World& ck3_world);
   // The country each CK3 ruler became, among those written to the mod.
   [[nodiscard]] std::map<long long, std::shared_ptr<Country>> MapCountriesByRuler() const;
   // Vassals cannot outrank their liege, so ranks are settled after every country exists.
   void AssignRanks();
   // Carries CK3 development onto EU5 and derives a technology level from it.
   void AssignDevelopment();

   // The steps each CK3 ruler goes through on its way to becoming a country, in the order the
   // constructor runs them.
   struct Context;
   struct TagChoice
   {
      std::string tag;
      bool generated = false;
   };
   // What a CK3 county carries onto every EU5 location made from it.
   struct CountyData
   {
      std::string religion;
      int development = -1;
      std::shared_ptr<ck3::Culture> culture;
   };
   struct PopulationWeights
   {
      std::map<std::string, double> culture;
      std::map<std::string, double> religion;
   };

   // Active CK3 wars between independent countries, once every country and subject is known.
   void AssignWars(const Context& context);
   [[nodiscard]] static std::optional<std::string> ResolveCapitalLocation(const ck3::Realm& realm,
       const Context& context);
   [[nodiscard]] std::optional<TagChoice> ChooseTag(const ck3::Realm& realm,
       const std::optional<std::string>& capital_location,
       Context& context);
   void MarkForDefinition(Country& country, const TagChoice& tag_choice, const Context& context);
   void AssignCapitalFaithAndCulture(Country& country,
       const ck3::Realm& realm,
       const std::optional<std::string>& capital_location,
       const Context& context);
   void AddCounty(Country& country, const ck3::Title& county, Context& context);
   [[nodiscard]] CountyData ReadCountyData(const ck3::Title& county, const Context& context);
   // The EU5 religion for a CK3 faith, falling back to its nearest mapped relative for the faiths a
   // campaign creates and the few religion_map misses.
   [[nodiscard]] std::optional<std::string> MapFaith(const ck3::Faith& faith, const Context& context);
   void ClaimLocation(Country& country,
       const std::string& location,
       const CountyData& county_data,
       const Context& context);
   [[nodiscard]] PopulationWeights WeighPopulation(const Country& country, const Context& context) const;
   void AssignPopulationFaithAndCulture(Country& country, const Context& context);

   void LogLandReport() const;
   void LogTagReport() const;
   void LogFaithAndCultureReport() const;
   void LogCountryList() const;

   std::vector<std::shared_ptr<Country>> countries_;
   date conversion_date_ = date("1.1.1");
   std::vector<std::string> ck3_trait_names_;
   // EU5 location to the religion of the CK3 county it was converted from.
   std::map<std::string, std::string> location_religions_;
   // EU5 location to the culture converted from the CK3 county, where the two disagree.
   std::map<std::string, std::string> location_cultures_;
   CultureResolver culture_resolver_;
   std::vector<Dependency> dependencies_;
   std::set<std::pair<std::string, std::string>> alliances_;
   std::vector<ConvertedWar> wars_;
   int wars_skipped_ = 0;
   int overlords_raised_to_subject_nations_ = 0;
   std::map<long long, ConvertedDynasty> dynasties_;
   std::map<std::string, ck3::CoatOfArms> flags_;
   // EU5 location to the CK3 development of the county it came from, and the bonus that becomes.
   std::map<std::string, int> location_development_;
   std::map<std::string, int> development_bonuses_;
   int development_baseline_ = 0;

   std::size_t realms_without_tag_ = 0;
   std::size_t counties_without_baronies_ = 0;
   std::size_t landless_counties_ = 0;
   std::size_t religions_replaced_ = 0;
   std::size_t cultures_replaced_ = 0;
   std::size_t realms_with_generated_tag_ = 0;
   std::size_t duplicate_tags_regenerated_ = 0;
   std::size_t vassals_demoted_ = 0;
   std::size_t tributaries_ = 0;
   std::size_t tributaries_skipped_ = 0;
   std::size_t family_members_ = 0;
   std::size_t heirs_ = 0;
   // CK3 faiths with no religion of their own that took a relative's, by name.
   std::set<std::string> faiths_by_relative_;
   // CK3 faiths nothing maps, not even a relative, by name.
   std::set<std::string> unmapped_faiths_;
   std::set<std::string> undefined_tags_;
};

}  // namespace eu5

#endif  // EU5_WORLD_H
