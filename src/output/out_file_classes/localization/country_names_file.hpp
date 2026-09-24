#ifndef OUT_COUNTRY_NAMES_FILE_H
#define OUT_COUNTRY_NAMES_FILE_H

#include <external/commonItems/Localization/LocalizationDatabase.h>

#include <filesystem>
#include <string>

#include "src/eu5_world/eu5_world.hpp"
#include "src/output/out_file_classes/output_file.hpp"

namespace out
{

// Writes one language's localisation for everything the conversion names: countries and their
// adjectives, rulers, and the cultures it generates. Without it EU5 shows each as its raw key.
//
// Country and ruler names come from the save, so they are in whatever language CK3 was played in,
// and are the same in every file. Generated cultures take CK3's own name in each language CK3 ships.
class CountryNamesFile: public OutputFile
{
  public:
   CountryNamesFile(const std::string& name,
       FileWriter& file_writer,
       const eu5::EU5World& eu5_world,
       const commonItems::LocalizationDatabase& ck3_culture_names,
       std::string language);

   void Create(const std::filesystem::path& folder_path) override;

  private:
   const eu5::EU5World& eu5_world_;
   const commonItems::LocalizationDatabase& ck3_culture_names_;
   std::string language_;
};

}  // namespace out

#endif  // OUT_COUNTRY_NAMES_FILE_H
