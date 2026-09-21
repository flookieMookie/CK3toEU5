#ifndef EU5_WORLD_H
#define EU5_WORLD_H

#include <cstddef>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include "eu5_country.hpp"
#include "eu5_country_definitions.hpp"

namespace ck3
{
class CK3World;
}

namespace mappers
{
class Mappers;
}

namespace eu5
{

// Turns the independent CK3 realms into EU5 countries: a tag, a capital, and the EU5 locations the
// realm holds, walked down from its counties through their baronies.
class EU5World
{
  public:
   EU5World() = default;
   EU5World(const ck3::CK3World& ck3_world,
       const mappers::Mappers& mappers,
       const CountryDefinitions& country_definitions);

   [[nodiscard]] const auto& GetCountries() const { return countries_; }
   [[nodiscard]] auto GetRealmsWithoutTag() const { return realms_without_tag_; }
   [[nodiscard]] auto GetCountiesWithoutBaronies() const { return counties_without_baronies_; }

   void LogReport() const;

  private:
   std::vector<std::shared_ptr<Country>> countries_;

   std::size_t realms_without_tag_ = 0;
   std::size_t counties_without_baronies_ = 0;
   std::size_t landless_counties_ = 0;
   std::size_t realms_with_undefined_tag_ = 0;
   std::set<std::string> undefined_tags_;
};

}  // namespace eu5

#endif  // EU5_WORLD_H
