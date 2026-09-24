#ifndef OUT_CHARACTERS_FILE_H
#define OUT_CHARACTERS_FILE_H

#include <filesystem>
#include <string>

#include "src/eu5_world/eu5_vanilla_characters.hpp"
#include "src/eu5_world/eu5_vanilla_countries.hpp"
#include "src/eu5_world/eu5_world.hpp"
#include "src/output/out_file_classes/output_file.hpp"

namespace out
{

// Writes setup/start characters: one ruler per converted country, taken from the CK3 realm holder.
// Without this every converted country is led by whoever vanilla put there in 1337.
//
// This replaces EU5's own file, so the characters of the vanilla countries kept on land CK3 doesn't
// cover are carried over too - their country blocks name them as rulers, heirs and regents.
class CharactersFile: public OutputFile
{
  public:
   CharactersFile(const std::string& name,
       FileWriter& file_writer,
       const eu5::EU5World& eu5_world,
       const eu5::VanillaCountries& vanilla_countries,
       const eu5::VanillaCharacters& vanilla_characters);

   void Create(const std::filesystem::path& folder_path) override;

  private:
   const eu5::EU5World& eu5_world_;
   const eu5::VanillaCountries& vanilla_countries_;
   const eu5::VanillaCharacters& vanilla_characters_;
};

}  // namespace out

#endif  // OUT_CHARACTERS_FILE_H
