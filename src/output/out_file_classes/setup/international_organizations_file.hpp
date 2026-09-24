#ifndef OUT_INTERNATIONAL_ORGANIZATIONS_FILE_H
#define OUT_INTERNATIONAL_ORGANIZATIONS_FILE_H

#include <filesystem>
#include <map>
#include <optional>
#include <set>
#include <string>

#include "src/eu5_world/eu5_vanilla_characters.hpp"
#include "src/eu5_world/eu5_vanilla_countries.hpp"
#include "src/eu5_world/eu5_world.hpp"
#include "src/output/out_file_classes/output_file.hpp"

namespace out
{

// Who may belong to EU5's own international organisations in a converted world.
struct OrganizationMembership
{
   // The vanilla countries kept on land CK3 doesn't cover, which carry their 1337 memberships over.
   std::set<std::string> kept_tags;
   // Converted countries by tag, with their religion. One sharing a tag with a vanilla member keeps
   // that membership only in a religious organisation of its own religion - a converted Byzantium
   // stays in the Patriarchate of Constantinople while it is still Orthodox.
   std::map<std::string, std::string> converted_religions;
   // Every character the mod defines, so a ruler_term naming one that is gone can be removed.
   std::set<std::string> characters;
};

// One add_international_organization entry, fitted to the converted world: members that no longer
// qualify are removed, ruler terms of characters that no longer exist are removed, and the whole
// organisation goes if its leader doesn't survive, nobody is left, or it names any other country
// that isn't a member in good standing. Empty when it goes.
[[nodiscard]] std::optional<std::string> FitOrganization(const std::string& organization,
    const OrganizationMembership& membership);

// Writes EU5's international organisations - churches, patriarchates, sects, the HRE, the
// Ilkhanate - fitted to the converted world. Left as they were, they named countries the conversion
// replaced, which EU5 rejects as countries that don't exist.
class InternationalOrganizationsFile: public OutputFile
{
  public:
   InternationalOrganizationsFile(const std::string& name,
       FileWriter& file_writer,
       const eu5::EU5World& eu5_world,
       const eu5::VanillaCountries& vanilla_countries,
       const eu5::VanillaCharacters& vanilla_characters,
       std::filesystem::path eu5_directory);

   void Create(const std::filesystem::path& folder_path) override;

  private:
   const eu5::EU5World& eu5_world_;
   const eu5::VanillaCountries& vanilla_countries_;
   const eu5::VanillaCharacters& vanilla_characters_;
   std::filesystem::path eu5_directory_;
};

}  // namespace out

#endif  // OUT_INTERNATIONAL_ORGANIZATIONS_FILE_H
