#ifndef OUT_COUNTRY_DEFINITIONS_FILE_H
#define OUT_COUNTRY_DEFINITIONS_FILE_H

#include <filesystem>
#include <string>

#include "src/eu5_world/eu5_vanilla_countries.hpp"
#include "src/eu5_world/eu5_world.hpp"
#include "src/output/out_file_classes/output_file.hpp"

namespace out
{

// Writes setup/countries definitions for converted tags EU5 does not define itself, so the game
// accepts them in 10_countries.txt instead of rejecting the block. A converted country reusing one
// of EU5's tags takes its culture and religion from that tag's definition, so EU5's own definition
// files that hold such tags are written again with the converted culture and religion.
class CountryDefinitionsFile: public OutputFile
{
  public:
   CountryDefinitionsFile(const std::string& name,
       FileWriter& file_writer,
       const eu5::EU5World& eu5_world,
       const eu5::VanillaCountries& vanilla_countries);

   void Create(const std::filesystem::path& folder_path) override;

  private:
   // EU5's definition files rewritten for the tags converted countries took over.
   void WriteRedefinedTags(const std::filesystem::path& folder_path);

   const eu5::EU5World& eu5_world_;
   const eu5::VanillaCountries& vanilla_countries_;
};

}  // namespace out

#endif  // OUT_COUNTRY_DEFINITIONS_FILE_H
