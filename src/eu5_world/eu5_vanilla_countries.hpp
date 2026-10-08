#ifndef EU5_VANILLA_COUNTRIES_H
#define EU5_VANILLA_COUNTRIES_H

#include <filesystem>
#include <map>
#include <set>
#include <string>
#include <vector>

#include "eu5_country_setup.hpp"

namespace eu5
{

// A country exactly as EU5 writes it in its own 10_countries.txt, kept as raw text.
struct VanillaCountry
{
   std::string tag;
   std::string block;                // the whole TAG = { ... } block, verbatim
   std::set<std::string> locations;  // every location the block claims
   std::set<std::string> owned;      // those it owns rather than only controls or has cores on
};

// EU5's own starting countries.
//
// The converter replaces 10_countries.txt wholesale, which silently unowns everything CK3 does not
// cover - the Americas, Oceania, much of Siberia. EU5 then complains about buildings and markets in
// locations with no owner, and those regions have no inhabitants at all. Countries whose land the
// conversion never touches are carried over verbatim instead.
class VanillaCountries
{
  public:
   VanillaCountries() = default;
   explicit VanillaCountries(const std::filesystem::path& eu5_directory);

   [[nodiscard]] const auto& GetCountries() const { return countries_; }
   // The templates, definitions and religion groups the countries are set up with.
   [[nodiscard]] const auto& GetSetup() const { return setup_; }
   // The countries none of whose land the conversion took, which are carried over as they are. One
   // it took any land from has been replaced by a converted country.
   [[nodiscard]] std::vector<const VanillaCountry*> GetUntouched(
       const std::set<std::string>& converted_locations) const;
   // The rest, which don't exist in the converted world: neither untouched nor reused by a converted
   // country. Those vanilla itself starts without land are among them.
   [[nodiscard]] std::vector<const VanillaCountry*> GetNotPresent(const std::set<std::string>& converted_locations,
       const std::set<std::string>& converted_tags) const;

  private:
   void Parse(const std::filesystem::path& file_path);

   std::vector<VanillaCountry> countries_;
   CountrySetup setup_;
};

}  // namespace eu5

#endif  // EU5_VANILLA_COUNTRIES_H
