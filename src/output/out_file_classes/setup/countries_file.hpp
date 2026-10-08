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

// The EU5 heir selection for a converted country. A monarchy's comes from its CK3 ruler's realm laws:
// partition becomes EU5's partition inheritance, which likewise weakens the realm at each succession;
// otherwise the gender law picks the primogeniture. Tribes, republics and theocracies take what EU5's
// own countries of their kind use.
[[nodiscard]] std::string HeirSelectionFor(const std::string& government, const std::set<std::string>& ck3_laws);

// The marriage law EU5 asks every country for: celibacy for theocracies, Muslim marriage in Islam,
// monogamy otherwise.
[[nodiscard]] std::string MarriageLawFor(const std::string& government,
    const std::optional<std::string>& religion_group);

// The heir religion law that goes with an heir selection.
[[nodiscard]] std::string HeirReligionLawFor(const std::string& heir_selection);

// A vanilla country block as EU5 writes a country that doesn't exist at the start: everything it
// held and gathered pops from is gone, and so are its rulers, past and present, who aren't in the
// converted world. Its government, capital and exploration stay; one with no capital named takes the
// first place it held, and knows that place's region.
[[nodiscard]] std::string NotPresentBlock(const std::string& block, const eu5::MapAreas& map_areas);

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

// The religious school a converted country of a religion that has them - Sunni, Shia, Ibadi - must
// name. EU5 lists them in in_game/common/religious_schools; these need no society values, which
// converted countries leave neutral (Hanafi, Maliki and the other jurisprudence schools need
// mysticism_vs_jurisprudence of 50). Empty for religions without schools.
[[nodiscard]] std::optional<std::string> ReligiousSchoolFor(const std::optional<std::string>& religion);

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
