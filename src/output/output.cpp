#include "output.hpp"

#include <external/commonItems/ConverterVersion.h>

#include <filesystem>
#include <memory>
#include <string>
#include <utility>

#include "out_file_classes/metadata/metadata.hpp"
#include "out_file_classes/output_folder.hpp"
#include "out_file_classes/setup/countries_file.hpp"



namespace out
{

Output::Output(std::string name, commonItems::ConverterVersion& converter_version, const eu5::EU5World& eu5_world):
    mod_name_(std::move(name)),
    converter_version_(std::move(converter_version)),
    output_path_(std::filesystem::path("output"))
{
   // ----------------------------------------------------------------------------------------
   // Here all the OutputFolder and OutputFileOrResource objects representing final mod files need to be created and
   // registered according to folder structure
   // ----------------------------------------------------------------------------------------
   root_folder_ = std::make_unique<OutputFolder>(output_path_.string(), folder_manager_);

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

   auto countries_file = std::make_unique<CountriesFile>("10_countries.txt", file_writer_, eu5_world);
   start_folder->RegisterFileOrResource(std::move(countries_file));

   setup_folder->RegisterSubfolder(std::move(start_folder));
   main_menu_folder->RegisterSubfolder(std::move(setup_folder));
   mod_folder->RegisterSubfolder(std::move(main_menu_folder));

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