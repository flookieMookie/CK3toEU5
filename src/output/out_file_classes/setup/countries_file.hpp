#ifndef OUT_COUNTRIES_FILE_H
#define OUT_COUNTRIES_FILE_H

#include <filesystem>
#include <optional>
#include <set>
#include <string>
#include <vector>

#include "src/eu5_world/eu5_map_areas.hpp"
#include "src/eu5_world/eu5_vanilla_countries.hpp"
#include "src/eu5_world/eu5_world.hpp"
#include "src/output/out_file_classes/output_file.hpp"

namespace out
{

// The EU5 heir selection for a converted monarchy, from its CK3 ruler's realm laws: partition becomes
// EU5's partition inheritance, which likewise weakens the realm at each succession; otherwise the
// gender law picks the primogeniture. Empty for other governments, which keep EU5's default.
[[nodiscard]] std::optional<std::string> HeirSelectionFor(const std::string& government,
    const std::set<std::string>& ck3_laws);

// What a converted country has discovered of the world, as lines of its country block. Without any,
// EU5 shows the player nothing but their own land. It takes the exploration of the country that held
// its capital in EU5's 1337 - vanilla's include = "expl_..." templates - and knows every region its
// own land lies in.
[[nodiscard]] std::string WriteDiscoveries(const std::vector<std::string>& locations,
    const std::string& capital_owner_block,
    const eu5::MapAreas& map_areas);

// Where a converted country sits between centralised and decentralised - EU5's own feudal monarchies
// start around +40, decentralised - from its CK3 ruler's crown or tribal authority law: the weaker
// the ruler's hold on the vassals, the more decentralised. Without either law, a default by government.
[[nodiscard]] int CentralizationFor(const std::string& government, const std::set<std::string>& ck3_laws);

// Writes setup/start/10_countries.txt: which EU5 locations each country owns at the start date.
class CountriesFile: public OutputFile
{
  public:
   CountriesFile(const std::string& name,
       FileWriter& file_writer,
       const eu5::EU5World& eu5_world,
       const eu5::VanillaCountries& vanilla_countries,
       const eu5::MapAreas& map_areas);

   void Create(const std::filesystem::path& folder_path) override;

  private:
   const eu5::EU5World& eu5_world_;
   const eu5::VanillaCountries& vanilla_countries_;
   const eu5::MapAreas& map_areas_;
};

}  // namespace out

#endif  // OUT_COUNTRIES_FILE_H
