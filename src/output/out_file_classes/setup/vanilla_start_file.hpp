#ifndef OUT_VANILLA_START_FILE_H
#define OUT_VANILLA_START_FILE_H

#include <filesystem>
#include <map>
#include <functional>
#include <optional>
#include <set>
#include <string>
#include <vector>

#include "src/eu5_world/eu5_vanilla_characters.hpp"
#include "src/eu5_world/eu5_vanilla_countries.hpp"
#include "src/eu5_world/eu5_world.hpp"
#include "src/output/out_file_classes/output_file.hpp"

namespace out
{

// Walks the entries of one of EU5's own start files - anything beginning entry_depth braces in - and
// replaces each with what transform returns for it, dropping it when that is empty. Everything
// outside the entries is kept as it is.
[[nodiscard]] std::string TransformEntries(const std::string& contents,
    int entry_depth,
    const std::function<std::optional<std::string>(const std::string& entry)>& transform);

// Keeps the entries of one of EU5's own start files that concern only the given countries.
//
// The file is copied line by line; an entry is anything that begins at entry_depth braces in, and
// is kept only if every country tag it names - a three letter uppercase token - is allowed.
// Comments are ignored when looking for tags. Everything outside the entries is kept as it is.
[[nodiscard]] std::string KeepEntriesAbout(const std::string& contents,
    int entry_depth,
    const std::set<std::string>& allowed_tags);

// Every country tag - three letter uppercase token - named outside comments.
[[nodiscard]] std::set<std::string> TagsNamedIn(const std::string& text);

// The text with every mention of a character the mod doesn't define taken out: lines naming one as
// character = X - a saint, a pope's ruler term - go, and an artist = X is dropped from its work of
// art, which stays. EU5's own history names people who never lived in a world converted from CK3.
[[nodiscard]] std::string WithoutMissingCharacters(const std::string& text, const std::set<std::string>& characters);

// The ids of the vanilla characters the mod keeps: those the vanilla countries kept on land CK3
// doesn't cover need. See VanillaCharacters::KeptFor.
[[nodiscard]] std::set<std::string> KeptVanillaCharacters(const eu5::VanillaCharacters& vanilla_characters,
    const std::vector<const eu5::VanillaCountry*>& kept_countries);

// Who holds what in the converted world, for handing EU5's own buildings to their new owners.
struct BuildingOwnership
{
   // Location to the country holding it in 1337, and in the converted world.
   std::map<std::string, std::string> vanilla_owners;
   std::map<std::string, std::string> current_owners;
   // The vanilla countries kept as they are, and the converted countries' religions.
   std::set<std::string> kept_tags;
   std::map<std::string, std::string> religions;
};

// One of EU5's building entries - castle = { tag = SWE level = 1 location = stockholm } - fitted to the
// converted world. A building is the land's, so it passes to whoever holds its location now. One that
// was foreign owned in 1337, like a Hanseatic kontor, describes a relationship that doesn't carry
// over, so it stays only where both sides are kept vanilla countries; a cardinal's seat only where
// the new owner is Catholic. Entries naming no owner are kept as they are. Empty when it goes.
[[nodiscard]] std::optional<std::string> FitBuilding(const std::string& entry, const BuildingOwnership& ownership);

// Writes one of EU5's start files - wars, rivals, opinions, armies, AI personalities - keeping only
// what concerns the vanilla countries the conversion keeps on land CK3 doesn't cover.
//
// Left as they are, these carry EU5's 1337 into the converted world: a converted country that
// happens to share a tag with a vanilla one would start in a vanilla war, hate a vanilla rival, and
// march armies led by characters who no longer exist. The converted countries' own relations come
// from CK3 instead.
class VanillaStartFile: public OutputFile
{
  public:
   VanillaStartFile(const std::string& name,
       FileWriter& file_writer,
       const eu5::EU5World& eu5_world,
       const eu5::VanillaCountries& vanilla_countries,
       std::filesystem::path eu5_directory,
       int entry_depth,
       std::function<std::string()> converted_entries = {});

   void Create(const std::filesystem::path& folder_path) override;

  private:
   const eu5::EU5World& eu5_world_;
   const eu5::VanillaCountries& vanilla_countries_;
   std::filesystem::path eu5_directory_;
   int entry_depth_;
   // Entries of the converted world's own, added inside the file's outer block after EU5's.
   std::function<std::string()> converted_entries_;
};

// The buildings CK3 holdings had, as entries of EU5's building_manager, leaving out any the location
// already has in EU5's own start (existing) - and stockades where there is a castle.
[[nodiscard]] std::string WriteConvertedBuildings(const std::vector<eu5::ConvertedBuilding>& buildings,
    const std::string& existing);

// Writes one of EU5's start files about places - buildings in 07_cities_and_buildings, cardinals'
// seats and saints in 13_religion, works of art in 11_art - fitted to the converted world: each
// building passes to whoever holds its location now (see FitBuilding), and characters who no longer
// exist are taken out (see WithoutMissingCharacters).
class VanillaLocationsFile: public OutputFile
{
  public:
   VanillaLocationsFile(const std::string& name,
       FileWriter& file_writer,
       const eu5::EU5World& eu5_world,
       const eu5::VanillaCountries& vanilla_countries,
       const eu5::VanillaCharacters& vanilla_characters,
       std::filesystem::path eu5_directory,
       std::function<std::string(const std::string& fitted)> converted_entries = {});

   void Create(const std::filesystem::path& folder_path) override;

  private:
   const eu5::EU5World& eu5_world_;
   const eu5::VanillaCountries& vanilla_countries_;
   const eu5::VanillaCharacters& vanilla_characters_;
   // Entries of the converted world's own, added inside the file's last block, given EU5's.
   std::function<std::string(const std::string& fitted)> converted_entries_;
   std::filesystem::path eu5_directory_;
};

}  // namespace out

#endif  // OUT_VANILLA_START_FILE_H
