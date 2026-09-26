#ifndef CK3_WORLD_H
#define CK3_WORLD_H

#include <Date.h>

#include "GameVersion.h"
#include "ModLoader/Mod.h"
#include "Parser.h"
#include "characters/characters.hpp"
#include "coats_of_arms/coats_of_arms.hpp"
#include "confederations/confederations.hpp"
#include "contracts/vassal_contracts.hpp"
#include "council_manager/councillor_tasks.hpp"
#include "cultures/cultures.hpp"
#include "dynasties/dynasties.hpp"
#include "flags/flags.hpp"
#include "geography/county_details.hpp"
#include "geography/province_holdings.hpp"
#include "realms/realms.hpp"
#include "relations/relations.hpp"
#include "wars/wars.hpp"
#include "armies/armies.hpp"
#include "religions/religions.hpp"
#include "src/configuration/configuration.hpp"
#include "titles/landed_titles.hpp"
#include "titles/titles.hpp"

namespace ck3
{
class CK3World
{
  public:
   explicit CK3World(const configuration::Configuration& configuration,
       const commonItems::ConverterVersion& converter_version);

   [[nodiscard]] const auto& GetCultures() const { return cultures_; }

   [[nodiscard]] const auto& GetConversionDate() const { return end_date_; }
   [[nodiscard]] const auto& GetTitles() const { return titles_; }
   [[nodiscard]] const auto& GetCharacters() const { return characters_; }
   [[nodiscard]] const auto& GetDynasties() const { return dynasties_; }
   [[nodiscard]] const auto& GetReligions() const { return religions_; }
   [[nodiscard]] const auto& GetConfederations() const { return confederations_; }
   [[nodiscard]] const auto& GetVassalContracts() const { return vassal_contracts_; }
   [[nodiscard]] const auto& GetRelations() const { return relations_; }
   [[nodiscard]] const auto& GetWars() const { return wars_; }
   [[nodiscard]] const auto& GetArmies() const { return armies_; }
   [[nodiscard]] const auto& GetProvinceHoldings() const { return province_holdings_; }
   // Trait names, indexed by the IDs characters' traits carry.
   [[nodiscard]] const auto& GetTraitNames() const { return trait_names_; }
   [[nodiscard]] const auto& GetCoatsOfArms() const { return coats_of_arms_; }
   [[nodiscard]] const auto& GetCountyDetails() const { return county_details_; }
   [[nodiscard]] const auto& GetLandedTitles() const { return landed_titles_; }
   [[nodiscard]] const auto& GetRealms() const { return realms_; }
   [[nodiscard]] const auto& GetMetaTitleName() const { return meta_realm_title_; }
   [[nodiscard]] const auto& GetUsedMods() const { return used_mods_; }
   // The save's mods that are installed here, in load order.
   [[nodiscard]] const auto& GetMods() const { return mods_; }
   //[[nodiscard]] const auto& GetMetaCoA() const { return metaCoA; }
   //[[nodiscard]] const auto& GetLocalizationMapper() const { return localizationMapper; }
   //[[nodiscard]] const auto& GetRivalPairs() const { return opinions.getRivalPairs(); }
   //[[nodiscard]] const auto& GetWars() const { return wars.getWars(); }
   //[[nodiscard]] const auto& GetArtifacts() const { return artifacts.getArtifacts(); }
   //[[nodiscard]] const auto& GetMenAtArms() const { return armies.getMenAtArms(); }

  private:
   void ParseGamestate(std::istream& input_stream, const commonItems::ConverterVersion& converter_version);
   void ParseMeta(std::istream& input_stream);
   void LoadMods(const configuration::Configuration& configuration);
   void LoadLandedTitles(const configuration::Configuration& configuration);

   // savegame processing

   date end_date_ = date("1444.11.11");
   date start_date_ = date("1.1.1");

   // meta
   std::optional<std::string> meta_realm_title_;
   // The mod descriptors the save lists.
   std::vector<std::string> used_mods_;

   GameVersion ck3_version_;
   Flags flags_;
   std::vector<Mod> mods_;

   // world
   Titles titles_;
   ProvinceHoldings province_holdings_;
   Characters characters_;
   Dynasties dynasties_;
   Religions religions_;
   CountyDetails county_details_;
   Cultures cultures_;
   // HouseNameScraper houseNameScraper;
   Confederations confederations_;
   VassalContracts vassal_contracts_;
   Relations relations_;
   Wars wars_;
   Armies armies_;
   std::vector<std::string> trait_names_;
   CoatsOfArms coats_of_arms_;
   CouncillorTasks councillor_tasks_;
   // Opinions opinions;
   // Wars wars;
   // Artifacts artifacts;
   // Armies armies;
   // mappers::NamedColors namedColors;
   // mappers::TraitScraper traitScraper;
   // mappers::LocalizationMapper localizationMapper;

   LandedTitles landed_titles_;

   Realms realms_;
};
}  // namespace ck3

#endif  // CK3_WORLD_H
