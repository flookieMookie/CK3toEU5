#include "characters_file.hpp"

#include <Date.h>
#include <external/commonItems/Log.h>

#include <set>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "src/ck3_world/characters/character.hpp"
#include "src/ck3_world/realms/realm.hpp"
#include "src/eu5_world/eu5_country.hpp"
#include "src/eu5_world/eu5_ruler_traits.hpp"

namespace
{
// EU5 has a single start date, START_DATE = "1337.4.1" in its defines, so a ruler born in 867
// would be five centuries old at game start.
// Birth dates are shifted by the gap between the CK3 save and this, preserving the ruler's age.
const date kGameStartDate = date("1337.4.1");

date AgeOntoStartDate(const date& birth_date, const date& conversion_date)
{
   date shifted = birth_date;
   shifted.ChangeByYears(kGameStartDate.getYear() - conversion_date.getYear());
   return shifted;
}

// A character's EU5 abilities, from the CK3 skills they correspond to, and the ruler traits their
// CK3 traits become.
void WriteAbilities(std::ostringstream& output, const ck3::Character& character, const std::vector<std::string>& trait_names)
{
   const auto& skills = character.GetSkills();
   const eu5::Abilities abilities{.adm = eu5::AbilityFromSkill(skills.stewardship),
       .dip = eu5::AbilityFromSkill(skills.diplomacy),
       .mil = eu5::AbilityFromSkill(skills.martial)};
   output << "\t\tadm = " << abilities.adm << " dip = " << abilities.dip << " mil = " << abilities.mil << "\n";

   std::set<std::string> ck3_traits;
   for (const auto trait_id: character.GetTraits())
   {
      if (trait_id >= 0 && static_cast<std::size_t>(trait_id) < trait_names.size())
      {
         ck3_traits.insert(trait_names[static_cast<std::size_t>(trait_id)]);
      }
   }
   for (const auto& trait: eu5::RulerTraitsFor(ck3_traits, abilities))
   {
      output << "\t\truler_trait = " << trait << "\n";
   }
}

// The CK3 nickname, as EU5 writes one: nickname = { name = key }. The text is in the localisation.
void WriteNickname(std::ostringstream& output, const ck3::Character& character)
{
   if (const auto key = eu5::NicknameKey(character.GetNickname(), character.GetNicknameText()); !key.empty())
   {
      output << "\t\tnickname = { name = \"" << key << "\" }\n";
   }
}
}  // namespace

namespace out
{

CharactersFile::CharactersFile(const std::string& name,
    FileWriter& file_writer,
    const eu5::EU5World& eu5_world,
    const eu5::VanillaCountries& vanilla_countries,
    const eu5::VanillaCharacters& vanilla_characters):
    OutputFile(name, file_writer),
    eu5_world_(eu5_world),
    vanilla_countries_(vanilla_countries),
    vanilla_characters_(vanilla_characters)
{
}

void CharactersFile::Create(const std::filesystem::path& folder_path)
{
   Log(LogLevel::Info) << "\tCreating " << GetName();

   std::ostringstream output;
   output << "# Rulers converted from the CK3 save.\n\n";
   output << "character_db = {\n";

   int written = 0;
   int family = 0;
   for (const auto& country: eu5_world_.GetCountries())
   {
      if (!country->IsWritten() || !country->HasRuler())
      {
         continue;
      }
      const auto& holder = country->GetSourceRealm()->GetHolder();

      output << "\n\t" << country->GetRulerId() << " = { # " << country->GetRulerName() << " of "
             << country->GetSourceRealm()->GetRealmName() << "\n";
      output << "\t\tfirst_name = { name = " << country->GetRulerNameKey() << " }\n";
      output << "\t\tculture = " << *country->GetCulture() << "\n";
      output << "\t\treligion = " << *country->GetReligion() << "\n";
      WriteAbilities(output, *holder, eu5_world_.GetCK3TraitNames());
      WriteNickname(output, *holder);
      if (holder->IsFemale())
      {
         output << "\t\tfemale = yes\n";
      }
      if (const auto dynasty = eu5_world_.DynastyIdOf(*holder); !dynasty.empty())
      {
         output << "\t\tdynasty = " << dynasty << "\n";
      }
      output << "\t\tbirth_date = "
             << AgeOntoStartDate(holder->GetBirthDate(), eu5_world_.GetConversionDate()).toString() << "\n";
      if (country->GetCapitalLocation().has_value())
      {
         output << "\t\tbirth = " << *country->GetCapitalLocation() << "\n";
      }
      output << "\t\ttag = " << country->GetTag() << "\n";
      output << "\t}\n";
      ++written;

      // The ruler's family shares their country's culture and religion, and is aged the same way.
      for (const auto& member: country->GetFamily())
      {
         const auto name = eu5::CleanCK3Name(member.character->GetName());
         output << "\n\t" << member.id << " = { # " << name << ", family of " << country->GetRulerName() << "\n";
         output << "\t\tfirst_name = { name = " << eu5::CharacterNameKey(name) << " }\n";
         output << "\t\tculture = " << *country->GetCulture() << "\n";
         output << "\t\treligion = " << *country->GetReligion() << "\n";
         WriteAbilities(output, *member.character, eu5_world_.GetCK3TraitNames());
         WriteNickname(output, *member.character);
         if (member.character->IsFemale())
         {
            output << "\t\tfemale = yes\n";
         }
         if (const auto dynasty = eu5_world_.DynastyIdOf(*member.character); !dynasty.empty())
         {
            output << "\t\tdynasty = " << dynasty << "\n";
         }
         output << "\t\tbirth_date = "
                << AgeOntoStartDate(member.character->GetBirthDate(), eu5_world_.GetConversionDate()).toString()
                << "\n";
         if (country->GetCapitalLocation().has_value())
         {
            output << "\t\tbirth = " << *country->GetCapitalLocation() << "\n";
         }
         for (const auto& [relation, id]: {std::pair{"father", member.father},
                  std::pair{"mother", member.mother},
                  std::pair{"spouse", member.spouse}})
         {
            if (!id.empty())
            {
               output << "\t\t" << relation << " = " << id << "\n";
            }
         }
         output << "\t\ttag = " << country->GetTag() << "\n";
         output << "\t}\n";
         ++family;
      }
   }

   // The vanilla countries kept on land CK3 doesn't cover name their own rulers, heirs and regents.
   int kept = 0;
   const auto kept_countries = vanilla_countries_.GetUntouched(eu5_world_.GetConvertedLocations());
   for (const auto& character: vanilla_characters_.KeptFor(kept_countries))
   {
      output << "\n" << character.block;
      ++kept;
   }

   output << "}\n";

   Log(LogLevel::Info) << "\t<> Wrote " << written << " rulers with " << family << " family members, and kept " << kept
                       << " vanilla characters of the countries CK3 doesn't cover.";
   UseFileWriter().CreateEmptyAndWrite(folder_path / GetName(), output.str());
}

}  // namespace out
