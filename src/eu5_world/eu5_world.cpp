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

// A county's de jure vassals are its baronies, and only baronies carry the province ID that the
// province mappings are keyed on.
std::vector<TitlePtr> BaroniesOf(const ck3::Title& county, const IdTitleMap& id_title_map)
{
   std::vector<TitlePtr> baronies;
   for (const auto& vassal: county.GetDeJureVassals())
   {
      const auto barony = id_title_map.find(vassal.GetID());
      if (barony != id_title_map.end() && barony->second->GetLevel() == ck3::Level::kBarony)
      {
         baronies.emplace_back(barony->second);
      }
   }
   return baronies;
}

long long ProvinceOf(const ck3::Title& barony, const ck3::LandedTitles& landed_titles)
{
   const auto landed_title = landed_titles.GetLandedTitles().find(barony.GetKey());
   if (landed_title == landed_titles.GetLandedTitles().end())
   {
      return -1;
   }
   return landed_title->second->GetProvince();
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
         for (const auto& barony: BaroniesOf(*realm->GetCapitalCounty(), id_title_map))
         {
            if (!barony->IsCountyCapital())
            {
               continue;
            }
            const auto& locations = province_mapper.GetEU5Locations(ProvinceOf(*barony, landed_titles));
            if (!locations.empty())
            {
               capital_location = locations.front();
            }
            break;
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
         const auto baronies = BaroniesOf(*county, id_title_map);
         if (baronies.empty())
         {
            ++counties_without_baronies_;
            continue;
         }
         for (const auto& barony: baronies)
         {
            for (const auto& location: province_mapper.GetEU5Locations(ProvinceOf(*barony, landed_titles)))
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
