#include "eu5_world.hpp"

#include <algorithm>
#include <map>
#include <set>
#include <string>
#include <utility>

#include "Log.h"
#include "src/ck3_world/ck3_world.hpp"
#include "src/ck3_world/realms/realm.hpp"
#include "src/ck3_world/realms/realms.hpp"
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

eu5::EU5World::EU5World(const ck3::CK3World& ck3_world, const mappers::Mappers& mappers)
{
   const auto id_title_map = MapTitlesById(ck3_world.GetTitles());
   const auto& landed_titles = ck3_world.GetLandedTitles();
   const auto& province_mapper = mappers.GetProvinceMapper();

   // An EU5 location can only belong to one country, so first come first served on overlaps.
   std::set<std::string> claimed_locations;

   for (const auto& realm: ck3_world.GetRealms().GetRealms())
   {
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

      const auto tag = mappers.GetTagMapper().GetEU5Tag(realm->GetPrimaryTitle()->GetKey(), capital_location);
      if (!tag.has_value())
      {
         ++realms_without_tag_;
         continue;
      }

      auto country = std::make_shared<Country>(*tag, realm);
      if (capital_location.has_value())
      {
         country->SetCapitalLocation(*capital_location);
      }
      const auto religion = mappers.GetReligionMapper().GetEU5Religion(realm->GetFaithName());
      if (religion.has_value())
      {
         country->SetReligion(*religion);
      }

      for (const auto& county: realm->GetCounties())
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
         for (const auto& barony_key: barony_keys)
         {
            for (const auto& location: province_mapper.GetEU5Locations(ProvinceOfBarony(barony_key, landed_titles)))
            {
               if (claimed_locations.insert(location).second)
               {
                  country->AddLocation(location);
               }
            }
         }
      }
      countries_.emplace_back(std::move(country));
   }

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
      Log(LogLevel::Warning) << "   " << realms_without_tag_ << " realms had no EU5 tag and were dropped.";
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
