#ifndef OUT_COATS_OF_ARMS_FILE_H
#define OUT_COATS_OF_ARMS_FILE_H

#include <filesystem>
#include <string>

#include "ModLoader/ModFilesystem.h"
#include "src/eu5_world/eu5_world.hpp"
#include "src/output/out_file_classes/output_file.hpp"

namespace out
{

// Writes common/coat_of_arms definitions for the countries whose tag the conversion invented, so
// they fly their CK3 arms instead of a blank flag. EU5 finds a country's arms by its tag.
class CoatsOfArmsFile: public OutputFile
{
  public:
   CoatsOfArmsFile(const std::string& name, FileWriter& file_writer, const eu5::EU5World& eu5_world);

   void Create(const std::filesystem::path& folder_path) override;

  private:
   const eu5::EU5World& eu5_world_;
};

// Supplies the art those arms need. CK3 and EU5 draw on different sets of patterns and emblems, and
// EU5 loads them by file name from its coat_of_arms folders, so any a converted flag uses that EU5
// lacks is copied from the player's own CK3 install into the mod.
class CoatOfArmsTextures: public OutputFile
{
  public:
   CoatOfArmsTextures(const std::string& name,
       FileWriter& file_writer,
       const eu5::EU5World& eu5_world,
       commonItems::ModFilesystem ck3_files,
       std::filesystem::path eu5_directory);

   // folder_path is the mod's gfx/coat_of_arms folder; the art goes into its subfolders.
   void Create(const std::filesystem::path& folder_path) override;

  private:
   const eu5::EU5World& eu5_world_;
   commonItems::ModFilesystem ck3_files_;
   std::filesystem::path eu5_directory_;
};

}  // namespace out

#endif  // OUT_COATS_OF_ARMS_FILE_H
