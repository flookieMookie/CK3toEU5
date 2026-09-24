#include "ck3_world.hpp"

#include <filesystem>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "CommonRegexes.h"
#include "Log.h"
#include "ModLoader/Mod.h"
#include "ModLoader/ModFilesystem.h"
#include "Parser.h"
#include "ParserHelpers.h"
#include "characters/characters.hpp"
#include "confederations/confederations.hpp"
#include "council_manager/councillor_tasks.hpp"
#include "cultures/cultures.hpp"
#include "dynasties/dynasties.hpp"
#include "external/commonItems/ConverterVersion.h"
#include "flags/flags.hpp"
#include "geography/county_details.hpp"
#include "geography/province_holdings.hpp"
#include "mods/ck3_mods.hpp"
#include "realms/realms.hpp"
#include "religions/religions.hpp"
#include "save_melter.hpp"
#include "src/configuration/configuration.hpp"

ck3::CK3World::CK3World(const configuration::Configuration& configuration,
    const commonItems::ConverterVersion& converter_version)
{
   Log(LogLevel::Info) << "-> Verifying CK3 save.";
   // TODO(Kmiotek): move this to a seperate class

   ck3::SaveMelter::VerifySave(configuration.GetSaveGamePath());
   const SaveData save_game = ck3::SaveMelter::MeltSave(configuration.GetSaveGamePath(), configuration.GetDebug());
   Log(LogLevel::Progress) << "5 %";

   Log(LogLevel::Info) << "* Parsing Metadata *";
   auto metadata_stream = std::istringstream(save_game.metadata);
   ParseMeta(metadata_stream);
   Log(LogLevel::Progress) << "7 %";
   Log(LogLevel::Info) << "* Parsing Gamestate *";
   auto game_state_stream = std::istringstream(save_game.gamestate);
   ParseGamestate(game_state_stream, converter_version);

   Log(LogLevel::Progress) << "20 %";

   Log(LogLevel::Info) << "* Gamestate Parsing Complete, Parsing Game Files *";
   LoadMods(configuration);
   LoadLandedTitles(configuration);
   Log(LogLevel::Progress) << "25 %";

   Log(LogLevel::Info) << "* Parsing Complete, Weaving Internals *";
   Log(LogLevel::Progress) << "30 %";

   characters_.LinkCharacters();
   characters_.LinkCultures(cultures_);
   characters_.LinkFaiths(religions_);
   characters_.LinkHouses(dynasties_);
   characters_.LinkTitles(titles_, councillor_tasks_);

   councillor_tasks_.LinkCharacters(characters_);

   confederations_.LinkCharacters(characters_);
   confederations_.LinkHouses(dynasties_);

   dynasties_.LinkCharacters(characters_);
   dynasties_.LinkDynasties();

   county_details_.LinkCultures(cultures_);
   county_details_.LinkReligions(religions_);

   religions_.LinkTitles(titles_);
   religions_.LinkReligions();

   Log(LogLevel::Info) << "-> Determining independent realms.";
   realms_ = Realms(titles_, characters_, county_details_);
   realms_.LogRealmReport();

   Log(LogLevel::Info) << "*** Good-bye CK3, rest in peace. ***";
   Log(LogLevel::Progress) << "47 %";
}

void ck3::CK3World::ParseGamestate(std::istream& input_stream, const commonItems::ConverterVersion& converter_version)
{
   Log(LogLevel::Info) << "*** Hello CK3, Deus Vult! ***";

   commonItems::parser parser;

   parser.registerRegex("SAV.*", [](const std::string&, std::istream&) {
   });
   parser.registerKeyword("date", [this](const std::string&, std::istream& input_stream) {
      const commonItems::singleString date_string(input_stream);
      end_date_ = date(date_string.getString());
   });
   parser.registerKeyword("bookmark_date", [this](const std::string&, std::istream& input_stream) {
      const commonItems::singleString start_date_string(input_stream);
      start_date_ = date(start_date_string.getString());
   });
   parser.registerKeyword("version",
       [this, converter_version](const std::string&,  // NOLINT - Issues with parser error handling
           std::istream& input_stream) {
          const commonItems::singleString version_string(input_stream);
          ck3_version_ = GameVersion(version_string.getString());
          Log(LogLevel::Info) << "<> Savegame version: " << version_string.getString();

          if (converter_version.getMinSource() > ck3_version_)
          {
             Log(LogLevel::Error) << "Converter requires a minimum save from v"
                                  << converter_version.getMinSource().toShortString();
             throw std::runtime_error("Savegame vs converter version mismatch!");
          }
          if (!converter_version.getMaxSource().isLargerishThan(ck3_version_))
          {
             Log(LogLevel::Error) << "Converter requires a maximum save from v"
                                  << converter_version.getMaxSource().toShortString();
             throw std::runtime_error("Savegame vs converter version mismatch!");
          }
       });

   parser.registerKeyword("variables", [this](const std::string&, std::istream& input_stream) {
      Log(LogLevel::Info) << "-> Loading variable flags.";
      flags_ = Flags(input_stream);
      Log(LogLevel::Info) << "<> Loaded " << flags_.GetFlags().size() << " variable flags and "
                          << flags_.GetUnavailableDecisionFlags().size() << " unavailable decision flags.";
   });
   parser.registerKeyword("landed_titles", [this](const std::string&, std::istream& input_stream) {
      Log(LogLevel::Info) << "-> Loading titles.";
      titles_ = Titles(input_stream);
      Log(LogLevel::Info) << "<> Loaded " << titles_.GetTitles().size() << " titles: " << titles_.GetBaronies().size()
                          << " baronies, " << titles_.GetCounties().size() << " counties, "
                          << titles_.GetDuchies().size() << " duchies, " << titles_.GetKingdoms().size()
                          << " kingdoms, " << titles_.GetEmpires().size() << " empires, "
                          << titles_.GetHegemonies().size() << " hegemonies.";
   });
   parser.registerKeyword("provinces", [this](const std::string&, std::istream& input_stream) {
      Log(LogLevel::Info) << "-> Loading provinces.";
      province_holdings_ = ProvinceHoldings(input_stream);
      Log(LogLevel::Info) << "<> Loaded " << province_holdings_.GetProvinceHoldings().size() << " provinces.";
   });
   parser.registerKeyword("council_task_manager", [this](const std::string&, std::istream& input_stream) {
      Log(LogLevel::Info) << "-> Loading councillor tasks.";
      councillor_tasks_ = CouncillorTasks(input_stream);
      Log(LogLevel::Info) << "<> Loaded " << councillor_tasks_.GetCouncillorTasks().size() << " councillor tasks.";
   });

   parser.registerKeyword("living", [this](std::istream& input_stream) {
      Log(LogLevel::Info) << "-> Loading alive characters.";
      characters_.ParseCharacters(input_stream);
      Log(LogLevel::Info) << "<> Loaded " << characters_.GetAliveCharacters().size() << " living characters.";
      Log(LogLevel::Info) << "<> Loaded " << characters_.GetDeadCharacters().size() << " dead human memories.";
   });

   parser.registerKeyword("dead_unprunable", [this](const std::string&, std::istream& input_stream) {
      Log(LogLevel::Info) << "-> Loading dead people.";
      characters_.ParseCharacters(input_stream);
      Log(LogLevel::Info) << "<> Loaded " << characters_.GetAliveCharacters().size()
                          << " living characters including from dead_unprunable.";
      Log(LogLevel::Info) << "<> Loaded " << characters_.GetDeadCharacters().size()
                          << " dead human memories including from dead_unprunable.";
   });
   parser.registerKeyword("dynasties", [this](const std::string&, std::istream& input_stream) {
      Log(LogLevel::Info) << "-> Loading dynasties.";
      dynasties_ = Dynasties(input_stream);
      Log(LogLevel::Info) << "<> Loaded " << dynasties_.GetDynasties().size() << " dynasties and "
                          << dynasties_.GetHouses().size() << " houses.";
   });

   parser.registerKeyword("religion", [this](const std::string&, std::istream& input_stream) {
      Log(LogLevel::Info) << "-> Loading religions.";
      religions_ = Religions(input_stream);
      Log(LogLevel::Info) << "<> Loaded " << religions_.GetReligions().size() << " religions and "
                          << religions_.GetFaiths().size() << " faiths.";
   });
   parser.registerKeyword("county_manager", [this](const std::string&, std::istream& input_stream) {
      Log(LogLevel::Info) << "-> Loading county details.";
      county_details_ = CountyDetails(input_stream);
      Log(LogLevel::Info) << "<> Loaded " << county_details_.GetCountyDetails().size() << " county details.";
   });
   parser.registerKeyword("culture_manager", [this](const std::string&, std::istream& input_stream) {
      Log(LogLevel::Info) << "-> Loading cultures.";
      cultures_ = Cultures(input_stream);
      Log(LogLevel::Info) << "<> Loaded " << cultures_.GetCultures().size() << " cultures.";
   });
   parser.registerKeyword("confederation_manager", [this](const std::string&, std::istream& input_stream) {
      Log(LogLevel::Info) << "-> Loading confederations.";
      confederations_ = Confederations(input_stream);
      Log(LogLevel::Info) << "<> Loaded " << confederations_.GetConfederations().size() << " confederations.";
   });
   // TODO(Kmiotek): add opinions
   parser.registerKeyword("coat_of_arms", [this](const std::string&, std::istream& input_stream) {
      Log(LogLevel::Info) << "-> Loading coats of arms.";
      coats_of_arms_ = CoatsOfArms(input_stream);
      Log(LogLevel::Info) << "<> Loaded " << coats_of_arms_.GetCoatsOfArms().size() << " coats of arms.";
   });
   // Characters name their traits by position in this list.
   parser.registerKeyword("traits_lookup", [this](const std::string&, std::istream& input_stream) {
      trait_names_ = commonItems::getStrings(input_stream);
   });
   parser.registerKeyword("wars", [this](const std::string&, std::istream& input_stream) {
      Log(LogLevel::Info) << "-> Loading wars.";
      wars_ = Wars(input_stream);
      Log(LogLevel::Info) << "<> Loaded " << wars_.GetWars().size() << " active wars.";
   });
   parser.registerKeyword("relations", [this](const std::string&, std::istream& input_stream) {
      Log(LogLevel::Info) << "-> Loading relations.";
      relations_ = Relations(input_stream);
      Log(LogLevel::Info) << "<> Loaded " << relations_.GetAlliances().size() << " alliances and "
                          << relations_.GetTruces().size() << " truces.";
   });
   // registerKeyword("opinions", [this](const std::string&, std::istream& input_stream) {
   //	Log(LogLevel::Info) << "-> Loading opinions.";
   //	opinions = Opinions(input_stream);
   //	Log(LogLevel::Info) << "<> Loaded " << opinions.getRivalPairs().size() << " rivalries.";
   // });
   parser.registerKeyword("vassal_contracts", [this](const std::string&, std::istream& input_stream) {
      Log(LogLevel::Info) << "-> Loading vassal contracts.";
      vassal_contracts_ = VassalContracts(input_stream);
      Log(LogLevel::Info) << "<> Loaded " << vassal_contracts_.GetContracts().size() << " vassal contracts, "
                          << vassal_contracts_.GetTributaries().size() << " of them tributaries.";
   });
   parser.registerRegex(commonItems::catchallRegex, commonItems::ignoreItem);

   parser.parseStream(input_stream);
   parser.clearRegisteredKeywords();
}

void ck3::CK3World::ParseMeta(std::istream& input_stream)
{
   commonItems::parser meta_parser;
   // CK3 writes this key only when the save used mods, as their descriptors: "mod/ugc_2834284159.mod".
   meta_parser.registerKeyword("mods", [this](std::istream& input_stream) {
      used_mods_ = commonItems::getStrings(input_stream);
   });
   meta_parser.registerKeyword("meta_title_name", [this](std::istream& input_stream) {
      // The realm name as CK3 displays it (e.g. "the Yamamoto Empire") - dynamic nomad/adventurer
      // titles_ often carry a stale internal name, so this is the better source for the player realm.
      meta_realm_title_ = commonItems::singleString(input_stream).getString();
      Log(LogLevel::Info) << "Meta title name: " << meta_realm_title_.value_or("no meta title");
   });

   // meta_parser_.registerKeyword("meta_coat_of_arms", [this](std::istream& input_stream) {
   //	// The realm arms as CK3 displays them - for dynamic realms these are the house arms, not the title's.
   //	metaCoA = std::make_shared<CoatOfArms>(input_stream, 0);
   // });
   meta_parser.registerRegex(commonItems::catchallRegex, commonItems::ignoreItem);
   meta_parser.parseStream(input_stream);
   meta_parser.clearRegisteredKeywords();
}

void ck3::CK3World::LoadMods(const configuration::Configuration& configuration)
{
   if (used_mods_.empty())
   {
      return;
   }
   Log(LogLevel::Info) << "-> Loading the save's " << used_mods_.size() << " mod(s).";
   mods_ = ResolveMods(configuration.GetCK3DocDirectory(), used_mods_);
   for (const auto& mod: mods_)
   {
      Log(LogLevel::Info) << "	" << mod.name;
      if (ChangesMap(mod))
      {
         Log(LogLevel::Warning) << "!!! " << mod.name << " changes CK3's map. The converter's province mappings are "
                                << "for CK3's own map, so land will end up in the wrong places or nowhere.";
      }
   }
   if (mods_.size() < used_mods_.size())
   {
      Log(LogLevel::Warning) << "!!! " << used_mods_.size() - mods_.size() << " of the save's mods are not installed. "
                             << "Anything they added - titles, cultures, names - will be missing from the conversion.";
   }
   Log(LogLevel::Info) << "<> Loaded " << mods_.size() << " mod(s): their titles, culture and dynasty names and coat "
                       << "of arms art are used.";
}

void ck3::CK3World::LoadLandedTitles(const configuration::Configuration& configuration)
{
   Log(LogLevel::Info) << "-> Loading Landed Titles.";
   const auto mod_filesystem = CK3Files(configuration.GetCK3Directory(), mods_);
   for (const auto& file: mod_filesystem.GetAllFilesInFolder("common/landed_titles"))
   {
      if (file.extension() == ".txt")
      {
         landed_titles_.LoadTitles(file);
      }
   }
   Log(LogLevel::Info) << "<> Loaded " << landed_titles_.GetLandedTitles().size() << " landed titles.";
}
