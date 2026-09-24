#ifndef OUT_DYNASTIES_FILE_H
#define OUT_DYNASTIES_FILE_H

#include <filesystem>
#include <string>

#include "src/eu5_world/eu5_world.hpp"
#include "src/output/out_file_classes/output_file.hpp"

namespace out
{

// Writes setup/start dynasties: EU5's own, with a dynasty for every CK3 house a converted character
// belongs to appended.
//
// EU5's own are kept because the vanilla countries carried over on land CK3 doesn't cover still
// name vanilla characters, who belong to vanilla dynasties.
class DynastiesFile: public OutputFile
{
  public:
   DynastiesFile(const std::string& name,
       FileWriter& file_writer,
       const eu5::EU5World& eu5_world,
       std::filesystem::path eu5_directory);

   void Create(const std::filesystem::path& folder_path) override;

  private:
   const eu5::EU5World& eu5_world_;
   std::filesystem::path eu5_directory_;
};

}  // namespace out

#endif  // OUT_DYNASTIES_FILE_H
