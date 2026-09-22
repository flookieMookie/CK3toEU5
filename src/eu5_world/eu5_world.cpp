#include "eu5_world.hpp"

#include <algorithm>
#include <cctype>
#include <map>
#include <set>
#include <string>
#include <utility>

#include "Log.h"
#include "src/ck3_world/characters/characters.hpp"
#include "src/ck3_world/ck3_world.hpp"
#include "src/ck3_world/geography/county_detail.hpp"
#include "src/ck3_world/geography/county_details.hpp"
#include "src/ck3_world/realms/realm.hpp"
#include "src/ck3_world/realms/realms.hpp"
#include "src/ck3_world/religions/faith.hpp"
#include "src/ck3_world/titles/landed_title.hpp"
#include "src/ck3_world/titles/landed_titles.hpp"
#include "src/ck3_world/titles/title.hpp"
#include "src/ck3_world/titles/titles.hpp"
#include "src/mappers/mappers.hpp"

namespace
{
using TitlePtr = std::shared_ptr<ck3::Title>;
using IdTitleMap = std::map<long long, TitlePtr>;

IdTitleMap MapTitlesById(const ck3::Titles& titles)
{
   IdTitleMap id_title_map;
   for (const auto& title: titles.GetTitles())
   {
      id_title_map.insert(std::pair(title.second->GetID(), title.second));
   }
   return id_title_map;
}

long long ProvinceOfBarony(const std::string& barony_key, const ck3::LandedTitles& landed_titles)
{
   const auto landed_title = landed_titles.GetLandedTitles().find(barony_key);
   if (landed_title == landed_titles.GetLandedTitles().end())
   {
      return -1;
   }
   return landed_title->second->GetProvince();
}

// Only baronies carry the province ID the province mappings are keyed on. The save lists a county's
// baronies under de_jure_vassals, but leaves the field off entirely for many counties, so fall back
// to the landed_titles hierarchy from the game files, which is always complete.
std::vector<std::string> BaronyKeysOf(const ck3::Title& county,
    const IdTitleMap& id_title_map,
    const ck3::LandedTitles& landed_titles)
{
   std::vector<std::string> barony_keys;
   for (const auto& vassal: county.GetDeJureVassals())
   {
      const auto barony = id_title_map.find(vassal.GetID());
      if (barony != id_title_map.end() && barony->second->GetLevel() == ck3::Level::kBarony)
      {
         barony_keys.emplace_back(barony->second->GetKey());
      }
   }
   if (!barony_keys.empty())
   {
      return barony_keys;
   }
   for (const auto& child_key: landed_titles.GetChildren(county.GetKey()))
   {
      if (child_key.starts_with("b_"))
      {
         barony_keys.emplace_back(child_key);
      }
   }
   return barony_keys;
}

// Most of a CK3 save has no EU5 counterpart - single county tribal realms across Asia and Africa
// that tag_mappings could never sensibly list. Rather than leave their land unowned, they get a
// generated tag built from the title key, so c_dublin becomes DUB where that is free.
std::string GenerateTag(const std::string& title_key, const std::set<std::string>& used_tags)
{
   const auto underscore = title_key.find('_');
   const auto name = underscore == std::string::npos ? title_key : title_key.substr(underscore + 1);

   std::string core;
   for (const char character: name)
   {
      if (std::isalnum(static_cast<unsigned char>(character)) != 0)
      {
         core += static_cast<char>(std::toupper(static_cast<unsigned char>(character)));
      }
   }

   static const std::string kBase36 = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
   if (core.size() >= 3 && !used_tags.contains(core.substr(0, 3)))
   {
      return core.substr(0, 3);
   }
   // Keep the first two letters recognisable and vary the last.
   if (core.size() >= 2)
   {
      for (const char suffix: kBase36)
      {
         if (const auto candidate = core.substr(0, 2) + suffix; !used_tags.contains(candidate))
         {
            return candidate;
         }
      }
   }
   // Last resort, a purely synthetic tag. EU5 ships none beginning with X0.
   for (const char first: kBase36)
   {
      for (const char second: kBase36)
      {
         if (const auto candidate = std::string("X") + first + second; !used_tags.contains(candidate))
         {
            return candidate;
         }
      }
   }
   return {};
}

// Walks the de facto tree below a ruler's top titles. Counties reached become their land. A duchy
// or higher title held by somebody else stops the walk and is reported back as a vassal, so its
// land goes to the vassal rather than the liege. Counts are deliberately not promoted: EU5 would
// end up with roughly two thousand countries and the map would no longer resemble CK3's.
void GatherLandAndVassals(const std::vector<TitlePtr>& top_titles,
    long long holder_id,
    const std::map<long long, std::vector<TitlePtr>>& de_facto_children,
    std::vector<TitlePtr>& counties,
    std::map<long long, std::vector<TitlePtr>>& vassal_titles_by_holder)
{
   std::set<long long> visited;
   std::set<long long> top_title_ids;
   for (const auto& title: top_titles)
   {
      if (title)
      {
         top_title_ids.insert(title->GetID());
      }
   }

   std::vector<TitlePtr> to_visit = top_titles;
   while (!to_visit.empty())
   {
      const auto title = to_visit.back();
      to_visit.pop_back();
      if (!title || !visited.insert(title->GetID()).second)
      {
         continue;
      }

      const bool is_own_top_title = top_title_ids.contains(title->GetID());
      if (!is_own_top_title && title->GetHolder().has_value() && title->GetHolder()->GetID() != holder_id &&
          title->GetLevel() >= ck3::Level::kDuchy)
      {
         vassal_titles_by_holder[title->GetHolder()->GetID()].emplace_back(title);
         continue;
      }

      if (title->GetLevel() == ck3::Level::kCounty)
      {
         counties.emplace_back(title);
      }
      const auto children = de_facto_children.find(title->GetID());
      if (children != de_facto_children.end())
      {
         to_visit.insert(to_visit.end(), children->second.begin(), children->second.end());
      }
   }
}

// A ruler waiting to become a country. Independent realms seed the list and their vassals are
// appended as the tree is walked, so a vassal's own vassals are handled the same way.
struct PendingRealm
{
   std::shared_ptr<ck3::Realm> realm;
   std::vector<TitlePtr> top_titles;
   long long holder_id = 0;
   std::string liege_tag;  // empty when independent
};

// Vassals have no ck3::Realm of their own - Realms only builds independent ones - so one is put
// together here from the titles they hold under their liege.
std::shared_ptr<ck3::Realm> MakeVassalRealm(const std::vector<TitlePtr>& titles,
    long long holder_id,
    const ck3::CK3World& ck3_world,
    const IdTitleMap& id_title_map)
{
   const auto& characters = ck3_world.GetCharacters().GetAllCharacters();
   const auto holder = characters.find(holder_id);
   if (holder == characters.end() || titles.empty())
   {
      return nullptr;
   }

   // The highest tier title they hold under this liege stands as their primary.
   const auto primary = std::ranges::max_element(titles, {}, [](const TitlePtr& title) {
      return title->GetLevel();
   });

   auto realm = std::make_shared<ck3::Realm>(*primary, holder->second);
   for (const auto& title: titles)
   {
      realm->AddHeldTitle(title);
   }
   if ((*primary)->GetCapitalCounty().has_value())
   {
      const auto capital = id_title_map.find((*primary)->GetCapitalCounty()->GetID());
      if (capital != id_title_map.end())
      {
         realm->SetCapitalCounty(capital->second);
         const auto details = ck3_world.GetCountyDetails().GetCountyDetails().find(capital->second->GetKey());
         if (details != ck3_world.GetCountyDetails().GetCountyDetails().end())
         {
            realm->SetCapitalDetails(details->second);
         }
      }
   }
   return realm;
}

// The save flags the county capital on the barony itself, when it bothers to list the baronies.
std::string CapitalBaronyKey(const ck3::Title& county, const IdTitleMap& id_title_map)
{
   for (const auto& vassal: county.GetDeJureVassals())
   {
      const auto barony = id_title_map.find(vassal.GetID());
      if (barony != id_title_map.end() && barony->second->IsCountyCapital())
      {
         return barony->second->GetKey();
      }
   }
   return {};
}
}  // namespace

eu5::EU5World::EU5World(const ck3::CK3World& ck3_world,
    const mappers::Mappers& mappers,
    const GameDefinitions& game_definitions,
    const LocationData& location_data)
{
   conversion_date_ = ck3_world.GetConversionDate();

   const auto id_title_map = MapTitlesById(ck3_world.GetTitles());
   const auto& landed_titles = ck3_world.GetLandedTitles();
   const auto& province_mapper = mappers.GetProvinceMapper();

   // An EU5 location can only belong to one country, so first come first served on overlaps.
   std::set<std::string> claimed_locations;
   // Generated tags must avoid every tag EU5 ships as well as everything handed out so far.
   std::set<std::string> used_tags = game_definitions.GetTags();
   // Tags actually given to a converted country, which is what makes a mapping a duplicate.
   std::set<std::string> assigned_tags;

   // Reverse index of de facto lieges, so a ruler's land can be walked downwards.
   std::map<long long, std::vector<TitlePtr>> de_facto_children;
   for (const auto& entry: ck3_world.GetTitles().GetTitles())
   {
      if (const auto& liege = entry.second->GetDeFactoLiege(); liege.has_value())
      {
         de_facto_children[liege->GetID()].emplace_back(entry.second);
      }
   }

   // Independent realms first; each one's duchy or higher vassals get appended as they are found.
   std::vector<PendingRealm> pending;
   for (const auto& realm: ck3_world.GetRealms().GetRealms())
   {
      if (realm->GetPrimaryTitle() && realm->GetHolder())
      {
         pending.emplace_back(PendingRealm{realm, realm->GetHeldTitles(), realm->GetHolder()->GetID(), {}});
      }
   }

   for (std::size_t index = 0; index < pending.size(); ++index)
   {
      const auto realm = pending[index].realm;
      const auto liege_tag = pending[index].liege_tag;
      if (!realm->GetPrimaryTitle())
      {
         continue;
      }

      // Resolve the capital first: a capital match beats a title match when picking the tag.
      std::optional<std::string> capital_location;
      if (realm->GetCapitalCounty())
      {
         const auto& capital_county = *realm->GetCapitalCounty();
         auto capital_barony_key = CapitalBaronyKey(capital_county, id_title_map);
         const auto barony_keys = BaronyKeysOf(capital_county, id_title_map, landed_titles);
         // CK3 lists a county's capital barony first, which covers counties the save left bare.
         if (capital_barony_key.empty() && !barony_keys.empty())
         {
            capital_barony_key = barony_keys.front();
         }
         if (!capital_barony_key.empty())
         {
            const auto& locations =
                province_mapper.GetEU5Locations(ProvinceOfBarony(capital_barony_key, landed_titles));
            if (!locations.empty())
            {
               capital_location = locations.front();
            }
         }
      }

      auto tag = mappers.GetTagMapper().GetEU5Tag(realm->GetPrimaryTitle()->GetKey(), capital_location);
      bool generated_tag = false;
      if (!tag.has_value())
      {
         tag = GenerateTag(realm->GetPrimaryTitle()->GetKey(), used_tags);
         if (tag->empty())
         {
            ++realms_without_tag_;
            continue;
         }
         generated_tag = true;
         ++realms_with_generated_tag_;
      }
      else if (assigned_tags.contains(*tag))
      {
         // Two realms can map to the same tag - a title and a capital both pointing at it. The
         // second one needs its own, or it would silently take over the first one's land.
         tag = GenerateTag(realm->GetPrimaryTitle()->GetKey(), used_tags);
         if (tag->empty())
         {
            ++realms_without_tag_;
            continue;
         }
         generated_tag = true;
         ++duplicate_tags_regenerated_;
      }
      used_tags.insert(*tag);
      assigned_tags.insert(*tag);

      auto country = std::make_shared<Country>(*tag, realm);
      if (capital_location.has_value())
      {
         country->SetCapitalLocation(*capital_location);
      }

      // tag_mappings still carries EU4 era tags that EU5 never defines - the Ottomans are TUR, not
      // OTT. Writing an undefined tag makes EU5 reject that block, and a rejected block early in
      // the file takes the rest down with it, so those tags need a definition written for them.
      if (generated_tag || (game_definitions.IsLoaded() && !game_definitions.HasTag(*tag)))
      {
         country->SetNeedsDefinition(true);
         if (!generated_tag)
         {
            undefined_tags_.insert(*tag);
         }
      }

      // religion_map has the same EU4 era drift - shiite for shia, and religions EU5 simply does
      // not have. Anything EU5 would reject falls back to what it already believes the capital is.
      const auto vanilla_religion =
          capital_location.has_value() ? location_data.GetDominantReligion(*capital_location) : std::string{};
      const auto mapped_religion = mappers.GetReligionMapper().GetEU5Religion(realm->GetFaithName());
      if (mapped_religion.has_value() &&
          (!game_definitions.IsLoaded() || game_definitions.HasReligion(*mapped_religion)))
      {
         country->SetReligion(*mapped_religion);
      }
      else if (!vanilla_religion.empty())
      {
         if (mapped_religion.has_value())
         {
            ++religions_replaced_;
         }
         country->SetReligion(vanilla_religion);
      }

      // There is no CK3 culture to EU5 culture mapping at all - the configurables only cover
      // culture groups - so a generated definition takes the culture EU5 already has in the capital.
      if (capital_location.has_value())
      {
         const auto culture = location_data.GetDominantCulture(*capital_location);
         if (!culture.empty() && (!game_definitions.IsLoaded() || game_definitions.HasCulture(culture)))
         {
            country->SetCulture(culture);
         }
      }

      // Land is gathered here rather than taken from the realm, because a duchy or higher vassal
      // keeps its own and becomes a country in its own right.
      std::vector<TitlePtr> counties;
      std::map<long long, std::vector<TitlePtr>> vassal_titles_by_holder;
      GatherLandAndVassals(pending[index].top_titles,
          pending[index].holder_id,
          de_facto_children,
          counties,
          vassal_titles_by_holder);

      for (const auto& county: counties)
      {
         const auto barony_keys = BaronyKeysOf(*county, id_title_map, landed_titles);
         if (barony_keys.empty())
         {
            // CK3's Chinese noble family counties are flagged landless and hold no barony, so they
            // are expected to contribute nothing rather than being a gap in the conversion.
            const auto landed_title = landed_titles.GetLandedTitles().find(county->GetKey());
            if (landed_title != landed_titles.GetLandedTitles().end() && landed_title->second->IsLandless())
            {
               ++landless_counties_;
            }
            else
            {
               ++counties_without_baronies_;
            }
            continue;
         }
         // The county's own faith, not the ruler's, so religious minorities within a realm survive.
         std::string county_religion;
         const auto county_detail = ck3_world.GetCountyDetails().GetCountyDetails().find(county->GetKey());
         if (county_detail != ck3_world.GetCountyDetails().GetCountyDetails().end())
         {
            if (const auto faith = county_detail->second->GetFaith().GetPointer().lock(); faith)
            {
               const auto mapped = mappers.GetReligionMapper().GetEU5Religion(faith->GetTag());
               if (mapped.has_value() && (!game_definitions.IsLoaded() || game_definitions.HasReligion(*mapped)))
               {
                  county_religion = *mapped;
               }
            }
         }

         for (const auto& barony_key: barony_keys)
         {
            for (const auto& location: province_mapper.GetEU5Locations(ProvinceOfBarony(barony_key, landed_titles)))
            {
               if (claimed_locations.insert(location).second)
               {
                  country->AddLocation(location);
                  if (!county_religion.empty())
                  {
                     location_religions_.insert_or_assign(location, county_religion);
                  }
               }
            }
         }
      }
      // EU5 checks a country's religion and culture against its own population and complains for
      // every one that disagrees. The capital alone is a poor proxy, so now that the land is known
      // both are taken from the pops the country actually holds, weighted by size.
      std::map<std::string, double> culture_weight;
      std::map<std::string, double> religion_weight;
      for (const auto& location: country->GetLocations())
      {
         const auto pops = location_data.GetPops().find(location);
         if (pops == location_data.GetPops().end())
         {
            continue;
         }
         const auto converted = location_religions_.find(location);
         const auto vanilla_majority = location_data.GetDominantReligion(location);
         for (const auto& pop: pops->second)
         {
            culture_weight[pop.culture] += pop.size;
            // Pops get rewritten to the county's converted faith, so count what the game will see.
            const bool rewritten = converted != location_religions_.end() && pop.religion == vanilla_majority;
            religion_weight[rewritten ? converted->second : pop.religion] += pop.size;
         }
      }
      if (!culture_weight.empty())
      {
         const auto top = std::ranges::max_element(culture_weight, {}, [](const auto& entry) {
            return entry.second;
         });
         if (!game_definitions.IsLoaded() || game_definitions.HasCulture(top->first))
         {
            country->SetCulture(top->first);
         }
      }
      if (!religion_weight.empty())
      {
         const auto top = std::ranges::max_element(religion_weight, {}, [](const auto& entry) {
            return entry.second;
         });
         if (!game_definitions.IsLoaded() || game_definitions.HasReligion(top->first))
         {
            country->SetReligion(top->first);
         }
      }

      // Each vassal ruler becomes a country of its own, holding the land under its titles.
      for (const auto& [vassal_holder_id, vassal_titles]: vassal_titles_by_holder)
      {
         auto vassal_realm = MakeVassalRealm(vassal_titles, vassal_holder_id, ck3_world, id_title_map);
         if (vassal_realm)
         {
            pending.emplace_back(PendingRealm{vassal_realm, vassal_titles, vassal_holder_id, *tag});
         }
      }

      if (!liege_tag.empty())
      {
         country->SetLiegeTag(liege_tag);
         dependencies_.emplace_back(Dependency{liege_tag, *tag});
      }
      countries_.emplace_back(std::move(country));
   }

   AssignRanks();

   std::ranges::sort(countries_, [](const std::shared_ptr<Country>& lhs, const std::shared_ptr<Country>& rhs) {
      return lhs->GetLocations().size() > rhs->GetLocations().size();
   });
}

void eu5::EU5World::LogReport() const
{
   std::size_t total_locations = 0;
   std::size_t without_capital = 0;
   std::size_t without_religion = 0;
   for (const auto& country: countries_)
   {
      total_locations += country->GetLocations().size();
      without_capital += country->GetCapitalLocation().has_value() ? 0 : 1;
      without_religion += country->GetReligion().has_value() ? 0 : 1;
   }

   Log(LogLevel::Info) << "<> " << countries_.size() << " EU5 countries holding " << total_locations << " locations.";
   if (realms_without_tag_ > 0)
   {
      Log(LogLevel::Warning) << "   " << realms_without_tag_ << " realms could not be given any tag and were dropped.";
   }
   if (without_capital > 0)
   {
      Log(LogLevel::Warning) << "   " << without_capital << " countries have no EU5 capital location.";
   }
   if (without_religion > 0)
   {
      Log(LogLevel::Warning) << "   " << without_religion << " countries have no EU5 religion.";
   }
   if (counties_without_baronies_ > 0)
   {
      Log(LogLevel::Warning) << "   " << counties_without_baronies_ << " counties had no baronies to draw land from.";
   }
   if (realms_with_generated_tag_ > 0)
   {
      Log(LogLevel::Info) << "   " << realms_with_generated_tag_
                          << " realms had no mapping and were given a generated tag.";
   }
   if (duplicate_tags_regenerated_ > 0)
   {
      Log(LogLevel::Warning) << "   " << duplicate_tags_regenerated_
                             << " realms mapped to a tag already taken and were given a generated one.";
   }
   if (!undefined_tags_.empty())
   {
      std::string tag_list;
      for (const auto& tag: undefined_tags_)
      {
         tag_list += tag + " ";
      }
      Log(LogLevel::Info) << "   " << undefined_tags_.size()
                          << " tags are not defined by EU5 and get a generated definition: " << tag_list;
   }
   if (religions_replaced_ > 0)
   {
      Log(LogLevel::Warning) << "   " << religions_replaced_
                             << " countries had a mapped religion EU5 does not define, replaced with the capital's.";
   }

   for (const auto& country: countries_)
   {
      if (country->GetLocations().empty())
      {
         continue;
      }
      Log(LogLevel::Info) << "   " << country->GetTag() << " <- " << country->GetSourceRealm()->GetRealmName() << " ("
                          << country->GetSourceRealm()->GetPrimaryTitle()->GetKey() << ")"
                          << " | capital: " << country->GetCapitalLocation().value_or("none")
                          << " | religion: " << country->GetReligion().value_or("none")
                          << " | locations: " << country->GetLocations().size();
   }
}

namespace
{
// EU5 ranks, weakest first. A country's rank comes from its CK3 title tier.
const std::vector<std::string> kRanks = {"rank_county", "rank_duchy", "rank_kingdom", "rank_empire"};

std::size_t RankIndexFor(ck3::Level tier)
{
   switch (tier)
   {
      case ck3::Level::kEmpire:
      case ck3::Level::kHegemony:
         return 3;
      case ck3::Level::kKingdom:
         return 2;
      case ck3::Level::kDuchy:
         return 1;
      default:
         return 0;
   }
}
}  // namespace

void eu5::EU5World::AssignRanks()
{
   std::map<std::string, std::size_t> rank_index;
   for (const auto& country: countries_)
   {
      rank_index[country->GetTag()] = RankIndexFor(country->GetSourceRealm()->GetTier());
   }

   // EU5 refuses a vassal that outranks its liege - vassal.txt requires
   // country_rank_level >= scope:target.country_rank_level - and silently drops the relationship.
   // Walking lieges first is not enough, because a liege can itself be someone's vassal, so this
   // repeats until nothing more needs lowering.
   bool changed = true;
   while (changed)
   {
      changed = false;
      for (const auto& country: countries_)
      {
         const auto& liege_tag = country->GetLiegeTag();
         if (liege_tag.empty())
         {
            continue;
         }
         const auto liege = rank_index.find(liege_tag);
         if (liege == rank_index.end())
         {
            continue;
         }
         auto& own = rank_index[country->GetTag()];
         if (own > liege->second)
         {
            own = liege->second;
            ++vassals_demoted_;
            changed = true;
         }
      }
   }

   for (const auto& country: countries_)
   {
      country->SetRank(kRanks[rank_index[country->GetTag()]]);
   }
}
