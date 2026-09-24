#ifndef EU5_COUNTRY_H
#define EU5_COUNTRY_H

#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace ck3
{
class Character;
class Realm;
}

namespace eu5
{

// CK3 names come out of the melt with escapes. A codepoint is written as an underscore followed by
// four hex digits (Cui_6F3C is Cui + U+6F3C), which this restores. Characters the melt could not
// represent leave a bare underscore behind instead - E_thelred for AEthelred, SigfriT_ for
// Sigfrid - and those are unrecoverable, so the underscore is dropped and the stranded capital
// lowercased to keep the name readable.
[[nodiscard]] std::string CleanCK3Name(const std::string& name);

// The localisation key a character's cleaned first name is written under. Shared by everyone of the
// same name, so each is localised once.
[[nodiscard]] std::string CharacterNameKey(const std::string& clean_name);

// A country's starting treasury from its CK3 ruler's gold. The two games count money on much the same
// scale at their starts, so it carries over as it is, held within the range EU5's own start data uses -
// from -500, a country deep in debt, to 2500 - so a late campaign's hoard doesn't break the economy.
[[nodiscard]] int StartingGoldFor(double ck3_gold);

// An EU5 ability - adm, dip or mil, 0 to 100 - from the CK3 skill it corresponds to: stewardship,
// diplomacy, martial. The save holds base skills, which among CK3's landed characters average 5 with
// a spread of about 3; EU5's own characters average about 46 with a spread of about 20. The scale
// lines the two up, so an average CK3 ruler is an average EU5 one and a 14 is as good as EU5 gets.
[[nodiscard]] int AbilityFromSkill(int ck3_skill);

// Someone from the ruler's family converted alongside them. Parents and spouse are EU5 character
// IDs, set only where that person is converted too.
struct FamilyMember
{
   std::string id;
   std::shared_ptr<ck3::Character> character;
   std::string father;
   std::string mother;
   std::string spouse;
};

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
   // EU5 rejects a vassal that outranks its liege, so rank is decided once the liege is known.
   [[nodiscard]] const auto& GetRank() const { return rank_; }
   [[nodiscard]] auto GetTechnologyLevel() const { return technology_level_; }
   [[nodiscard]] const auto& GetLiegeTag() const { return liege_tag_; }
   [[nodiscard]] const auto& GetFamily() const { return family_; }
   // Empty when the heir isn't converted.
   [[nodiscard]] const auto& GetHeirId() const { return heir_id_; }
   // Whether the country makes it into the mod at all. One with no land is left out, and so is one
   // needing a definition that lacks the culture or religion a definition requires, since EU5 would
   // reject it. Anything else that names a country must check this, or it names one EU5 lacks.
   [[nodiscard]] bool IsWritten() const
   {
      return !locations_.empty() && (!needs_definition_ || (culture_.has_value() && religion_.has_value()));
   }

   // A ruler can only be written when the realm has a holder and EU5 will accept their culture and
   // religion, since a character naming something EU5 does not define is rejected.
   [[nodiscard]] bool HasRuler() const;
   // Stable per country, so 10_characters and 10_countries agree.
   [[nodiscard]] std::string GetRulerId() const;
   [[nodiscard]] std::string GetRulerName() const;
   // The localisation key the ruler's name is written under.
   [[nodiscard]] std::string GetRulerNameKey() const;

   void SetCapitalLocation(std::string location) { capital_location_ = std::move(location); }
   void SetReligion(std::string religion) { religion_ = std::move(religion); }
   void SetCulture(std::string culture) { culture_ = std::move(culture); }
   void SetNeedsDefinition(bool needs_definition) { needs_definition_ = needs_definition; }
   void SetTechnologyLevel(int level) { technology_level_ = level; }
   void SetRank(std::string rank) { rank_ = std::move(rank); }
   void SetLiegeTag(std::string liege_tag) { liege_tag_ = std::move(liege_tag); }
   void AddFamilyMember(FamilyMember member) { family_.push_back(std::move(member)); }
   void SetHeirId(std::string heir_id) { heir_id_ = std::move(heir_id); }
   void AddLocation(std::string location) { locations_.emplace_back(std::move(location)); }

  private:
   std::string tag_;
   std::shared_ptr<ck3::Realm> source_realm_;

   std::optional<std::string> capital_location_;
   std::optional<std::string> religion_;
   std::optional<std::string> culture_;
   bool needs_definition_ = false;
   std::string rank_ = "rank_county";
   int technology_level_ = 3;
   std::string liege_tag_;
   std::vector<FamilyMember> family_;
   std::string heir_id_;
   std::vector<std::string> locations_;
};

}  // namespace eu5

#endif  // EU5_COUNTRY_H
