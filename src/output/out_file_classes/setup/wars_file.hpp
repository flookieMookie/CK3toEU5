#ifndef OUT_WARS_FILE_H
#define OUT_WARS_FILE_H

#include <Date.h>

#include <filesystem>
#include <map>
#include <string>
#include <vector>

#include "src/eu5_world/eu5_map_areas.hpp"
#include "src/eu5_world/eu5_vanilla_countries.hpp"
#include "src/eu5_world/eu5_world.hpp"
#include "src/output/out_file_classes/output_file.hpp"

namespace out
{

// One converted war as an entry of EU5's war_manager, in the shape of EU5's own conquest wars:
// the leaders' war to take one location, with its start moved onto EU5's calendar the way converted
// rulers' birthdays are.
[[nodiscard]] std::string WriteWar(const eu5::ConvertedWar& war, const date& conversion_date);

// The levies raised for the converted wars, as EU5 writes them in 27_armies: every country fighting
// one raises its levy in the area of its capital, and it musters there - as EU5 starts Venice and
// Verona, the countries at war in 1337.
[[nodiscard]] std::string WriteLevies(const std::vector<eu5::ConvertedWar>& wars,
    const std::map<std::string, std::string>& capitals,
    const eu5::MapAreas& map_areas);

// Writes 16_wars.txt: EU5's own wars among the vanilla countries the conversion keeps, and the
// wars the CK3 save was in the middle of.
class WarsFile: public OutputFile
{
  public:
   WarsFile(const std::string& name,
       FileWriter& file_writer,
       const eu5::EU5World& eu5_world,
       const eu5::VanillaCountries& vanilla_countries,
       std::filesystem::path eu5_directory);

   void Create(const std::filesystem::path& folder_path) override;

  private:
   const eu5::EU5World& eu5_world_;
   const eu5::VanillaCountries& vanilla_countries_;
   std::filesystem::path eu5_directory_;
};

}  // namespace out

#endif  // OUT_WARS_FILE_H
