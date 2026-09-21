#ifndef EU5_COUNTRY_H
#define EU5_COUNTRY_H

#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace ck3
{
class Realm;
}

namespace eu5
{

// An EU5 country converted from one independent CK3 realm. Locations are EU5 location names, the
// keys 10_countries.txt is written in.
class Country
{
  public:
   Country(std::string tag, std::shared_ptr<ck3::Realm> source_realm);

   [[nodiscard]] const auto& GetTag() const { return tag_; }
   [[nodiscard]] const auto& GetSourceRealm() const { return source_realm_; }
   [[nodiscard]] const auto& GetCapitalLocation() const { return capital_location_; }
   [[nodiscard]] const auto& GetLocations() const { return locations_; }
   [[nodiscard]] const auto& GetReligion() const { return religion_; }
   [[nodiscard]] const auto& GetCulture() const { return culture_; }
   // True when EU5 has no definition for this tag and the converter must write one.
   [[nodiscard]] auto NeedsDefinition() const { return needs_definition_; }

   void SetCapitalLocation(std::string location) { capital_location_ = std::move(location); }
   void SetReligion(std::string religion) { religion_ = std::move(religion); }
   void SetCulture(std::string culture) { culture_ = std::move(culture); }
   void SetNeedsDefinition(bool needs_definition) { needs_definition_ = needs_definition; }
   void AddLocation(std::string location) { locations_.emplace_back(std::move(location)); }

  private:
   std::string tag_;
   std::shared_ptr<ck3::Realm> source_realm_;

   std::optional<std::string> capital_location_;
   std::optional<std::string> religion_;
   std::optional<std::string> culture_;
   bool needs_definition_ = false;
   std::vector<std::string> locations_;
};

}  // namespace eu5

#endif  // EU5_COUNTRY_H
