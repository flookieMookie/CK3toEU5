#include "output.hpp"

#include <external/commonItems/ConverterVersion.h>

#include <filesystem>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "out_file_classes/localization/country_names_file.hpp"
#include "out_file_classes/metadata/metadata.hpp"
#include "out_file_classes/output_folder.hpp"
#include "out_file_classes/setup/coats_of_arms_file.hpp"
#include "out_file_classes/setup/characters_file.hpp"
#include "out_file_classes/setup/countries_file.hpp"
#include "out_file_classes/setup/country_definitions_file.hpp"
#include "out_file_classes/setup/culture_definitions_file.hpp"
#include "out_file_classes/setup/development_file.hpp"
#include "out_file_classes/setup/diplomacy_file.hpp"
#include "out_file_classes/setup/dynasties_file.hpp"
#include "out_file_classes/setup/international_organizations_file.hpp"
#include "out_file_classes/setup/pops_file.hpp"
#include "out_file_classes/setup/vanilla_start_file.hpp"
#include "out_file_classes/setup/wars_file.hpp"



namespace
{
// The languages EU5 1.3 ships localisation for.
const std::vector<std::string> kEU5Languages =
    {"braz_por", "english", "french", "german", "japanese", "korean", "polish", "russian", "simp_chinese", "spanish", "turkish"};
}  // namespace

namespace out
{

Output::Output(std::string name,
    commonItems::ConverterVersion& converter_version,
    const eu5::EU5World& eu5_world,
    const eu5::LocationData& location_data,
    const eu5::VanillaCountries& vanilla_countries,
    const eu5::VanillaCharacters& vanilla_characters,
    const std::filesystem::path& eu5_directory,
    const commonItems::ModFilesystem& ck3_files,
    const commonItems::LocalizationDatabase& ck3_culture_names,
    const commonItems::LocalizationDatabase& ck3_dynasty_names,
    const commonItems::LocalizationDatabase& ck3_nicknames):
    mod_name_(std::move(name)),
    converter_version_(std::move(converter_version)),
    output_path_(std::filesystem::path("output"))
{
   // ----------------------------------------------------------------------------------------
   // Here all the OutputFolder and OutputFileOrResource objects representing final mod files need to be created and
   // registered according to folder structure
   // ----------------------------------------------------------------------------------------
   root_folder_ = std::make_unique<OutputFolder>(output_path_.string(), folder_manager_);
   map_areas_ = std::make_shared<const eu5::MapAreas>(eu5_directory);

   auto mod_folder = std::make_unique<OutputFolder>(mod_name_, folder_manager_);

   // Metadata
   auto metadata_folder = std::make_unique<OutputFolder>(".metadata", folder_manager_);

   auto metadata_file = std::make_unique<MetadataFile>(mod_name_, file_writer_, converter_version_);

   metadata_folder->RegisterFileOrResource(std::move(metadata_file));

   mod_folder->RegisterSubfolder(std::move(metadata_folder));

   // Setup
   // ----------------------------------
   // The mod mirrors the game's own folder layout, so the start files live under
   // main_menu/setup/start, matching game/main_menu/setup/start in the EU5 install.
   auto main_menu_folder = std::make_unique<OutputFolder>("main_menu", folder_manager_);
   auto setup_folder = std::make_unique<OutputFolder>("setup", folder_manager_);
   auto start_folder = std::make_unique<OutputFolder>("start", folder_manager_);

   auto countries_file =
       std::make_unique<CountriesFile>("10_countries.txt", file_writer_, eu5_world, vanilla_countries, *map_areas_);
   start_folder->RegisterFileOrResource(std::move(countries_file));

   auto dynasties_file =
       std::make_unique<DynastiesFile>("04_dynasties.txt", file_writer_, eu5_world, eu5_directory);
   start_folder->RegisterFileOrResource(std::move(dynasties_file));

   auto characters_file = std::make_unique<CharactersFile>(
       "05_characters.txt", file_writer_, eu5_world, vanilla_countries, vanilla_characters);
   start_folder->RegisterFileOrResource(std::move(characters_file));

   auto pops_file = std::make_unique<PopsFile>("06_pops.txt", file_writer_, eu5_world, location_data);
   start_folder->RegisterFileOrResource(std::move(pops_file));

   auto diplomacy_file = std::make_unique<DiplomacyFile>("12_diplomacy.txt", file_writer_, eu5_world);
   start_folder->RegisterFileOrResource(std::move(diplomacy_file));

   auto development_file =
       std::make_unique<DevelopmentFile>("14_development.txt", file_writer_, eu5_world, eu5_directory);
   start_folder->RegisterFileOrResource(std::move(development_file));

   start_folder->RegisterFileOrResource(std::make_unique<InternationalOrganizationsFile>(
       "15_international_organizations.txt", file_writer_, eu5_world, vanilla_countries, vanilla_characters, eu5_directory));

   // The wars the CK3 save was in the middle of, beside EU5's own among the countries it keeps.
   start_folder->RegisterFileOrResource(
       std::make_unique<WarsFile>("16_wars.txt", file_writer_, eu5_world, vanilla_countries, eu5_directory));

   // EU5's own rivalries, opinions, colonial claims and AI personalities, kept only for the
   // vanilla countries on land CK3 doesn't cover. Each entry sits one brace in, some two.
   for (const auto& [file_name, entry_depth]: {std::pair{"18_opinions.txt", 1},
            std::pair{"20_rivals.txt", 1},
            std::pair{"23_colonies.txt", 1},
            std::pair{"25_area_preferences.txt", 2},
            std::pair{"26_ai_personalities.txt", 2}})
   {
      start_folder->RegisterFileOrResource(std::make_unique<VanillaStartFile>(
          file_name, file_writer_, eu5_world, vanilla_countries, eu5_directory, entry_depth));
   }

   // EU5's own armies likewise, and the levies the countries fighting the converted wars have raised.
   start_folder->RegisterFileOrResource(std::make_unique<VanillaStartFile>("27_armies.txt",
       file_writer_,
       eu5_world,
       vanilla_countries,
       eu5_directory,
       1,
       [&eu5_world, map_areas = map_areas_]() {
          std::map<std::string, std::string> capitals;
          for (const auto& country: eu5_world.GetCountries())
          {
             if (country->IsWritten() && country->GetCapitalLocation().has_value())
             {
                capitals.emplace(country->GetTag(), *country->GetCapitalLocation());
             }
          }
          return WriteStandingArmies(eu5_world.GetStandingArmies(), capitals) +
                 WriteLevies(eu5_world.GetWars(), capitals, *map_areas);
       }));

   // EU5's own buildings, cardinals' seats, saints and works of art: buildings handed to whoever
   // holds their land now, people who never lived in the converted world taken out.
   for (const auto* file_name: {"07_cities_and_buildings.txt", "11_art.txt", "13_religion.txt"})
   {
      start_folder->RegisterFileOrResource(std::make_unique<VanillaLocationsFile>(
          file_name, file_writer_, eu5_world, vanilla_countries, vanilla_characters, eu5_directory));
   }

   setup_folder->RegisterSubfolder(std::move(start_folder));
   main_menu_folder->RegisterSubfolder(std::move(setup_folder));

   // Localisation sits under main_menu too, matching the game and its DLC. Every language EU5 ships
   // gets a file, or players outside English see raw keys where converted names should be.
   auto localization_folder = std::make_unique<OutputFolder>("localization", folder_manager_);
   for (const auto& language: kEU5Languages)
   {
      auto language_folder = std::make_unique<OutputFolder>(language, folder_manager_);
      auto names_file = std::make_unique<CountryNamesFile>("00_converted_names_l_" + language + ".yml",
          file_writer_,
          eu5_world,
          ck3_culture_names,
          ck3_dynasty_names,
          ck3_nicknames,
          language);
      language_folder->RegisterFileOrResource(std::move(names_file));
      localization_folder->RegisterSubfolder(std::move(language_folder));
   }
   main_menu_folder->RegisterSubfolder(std::move(localization_folder));

   // Coats of arms, also under main_menu as in the game: the definitions in common, and the CK3 art
   // they need that EU5 lacks in gfx.
   auto common_folder = std::make_unique<OutputFolder>("common", folder_manager_);
   auto common_coat_of_arms_folder = std::make_unique<OutputFolder>("coat_of_arms", folder_manager_);
   auto definitions_folder = std::make_unique<OutputFolder>("coat_of_arms", folder_manager_);
   definitions_folder->RegisterFileOrResource(
       std::make_unique<CoatsOfArmsFile>("00_converted_coats_of_arms.txt", file_writer_, eu5_world));
   common_coat_of_arms_folder->RegisterSubfolder(std::move(definitions_folder));
   common_folder->RegisterSubfolder(std::move(common_coat_of_arms_folder));
   main_menu_folder->RegisterSubfolder(std::move(common_folder));

   auto gfx_folder = std::make_unique<OutputFolder>("gfx", folder_manager_);
   auto art_folder = std::make_unique<OutputFolder>("coat_of_arms", folder_manager_);
   art_folder->RegisterFileOrResource(std::make_unique<CoatOfArmsTextures>(
       "coat_of_arms_art", file_writer_, eu5_world, ck3_files, eu5_directory));
   gfx_folder->RegisterSubfolder(std::move(art_folder));
   main_menu_folder->RegisterSubfolder(std::move(gfx_folder));

   mod_folder->RegisterSubfolder(std::move(main_menu_folder));

   // Country definitions live under in_game rather than main_menu, mirroring the game again.
   auto in_game_folder = std::make_unique<OutputFolder>("in_game", folder_manager_);
   auto in_game_setup_folder = std::make_unique<OutputFolder>("setup", folder_manager_);
   auto countries_folder = std::make_unique<OutputFolder>("countries", folder_manager_);

   auto country_definitions_file =
       std::make_unique<CountryDefinitionsFile>("00_converted_countries.txt", file_writer_, eu5_world);
   countries_folder->RegisterFileOrResource(std::move(country_definitions_file));

   in_game_setup_folder->RegisterSubfolder(std::move(countries_folder));
   in_game_folder->RegisterSubfolder(std::move(in_game_setup_folder));

   // Generated cultures live under in_game/common/cultures, mirroring the game again.
   auto in_game_common_folder = std::make_unique<OutputFolder>("common", folder_manager_);
   auto cultures_folder = std::make_unique<OutputFolder>("cultures", folder_manager_);

   auto culture_definitions_file =
       std::make_unique<CultureDefinitionsFile>("00_converted_cultures.txt", file_writer_, eu5_world);
   cultures_folder->RegisterFileOrResource(std::move(culture_definitions_file));

   in_game_common_folder->RegisterSubfolder(std::move(cultures_folder));
   in_game_folder->RegisterSubfolder(std::move(in_game_common_folder));
   mod_folder->RegisterSubfolder(std::move(in_game_folder));

   // The scaffold also registered a history/advisors.txt holding the placeholder "zaba 123 321" and
   // copied resources/localisation, whose only content is a 3 byte stub. Both shipped inside the
   // generated mod. AdvisorFile and CopyResource are kept and still tested; they just need real
   // content before they go back into the output.

   // ----------------------------------
   root_folder_->RegisterSubfolder(std::move(mod_folder));
}

void Output::GenerateOutputMod()
{
   folder_manager_.RemoveFolder(output_path_);  // Removes previous output folder

   root_folder_->CreateRecursive(
       std::filesystem::path(""));  // Creates all mod files in the newly created output folder
}


}  // namespace out