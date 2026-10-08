#ifndef EU5_COUNTRY_SETUP_H
#define EU5_COUNTRY_SETUP_H

#include <filesystem>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace eu5
{

// A country definition file of EU5's own, in_game/setup/countries/<name>, as text.
struct DefinitionFile
{
   std::string name;
   std::string text;
};

// What EU5 sets its countries up with besides their 10_countries blocks: the templates those blocks
// include - main_menu/setup/templates, holding a government's laws, estate privileges and parliament -
// the country definitions naming each tag's culture and religion, and the group each religion is in.
class CountrySetup
{
  public:
   CountrySetup() = default;
   explicit CountrySetup(const std::filesystem::path& eu5_directory);

   void AddTemplate(const std::string& name, const std::string& text) { templates_[name] = text; }
   void AddDefinitionFile(const std::string& name, const std::string& text);
   void AddReligions(const std::string& text);

   // The government type - monarchy, republic, theocracy, tribe, steppe_horde - a country block sets,
   // itself or through the templates it includes.
   [[nodiscard]] std::optional<std::string> GovernmentTypeOf(const std::string& block) const;
   // The templates a country block includes that describe its government rather than its exploration.
   [[nodiscard]] std::vector<std::string> GovernmentTemplatesOf(const std::string& block) const;
   // The landlocked variant of a template where EU5 has one: the coastal ones add naval laws, which a
   // country without a port may not hold.
   [[nodiscard]] std::string LandlockedVariantOf(const std::string& template_name) const;

   // A tag's culture_definition and religion_definition, where EU5 defines the tag.
   [[nodiscard]] std::optional<std::string> CultureOf(const std::string& tag) const;
   [[nodiscard]] std::optional<std::string> ReligionOf(const std::string& tag) const;
   // The group a religion belongs to - christian, muslim, dharmic and so on.
   [[nodiscard]] std::optional<std::string> GroupOf(const std::string& religion) const;

   [[nodiscard]] const auto& GetDefinitionFiles() const { return definition_files_; }

  private:
   std::map<std::string, std::string> templates_;
   std::vector<DefinitionFile> definition_files_;
   std::map<std::string, std::string> culture_of_tag_;
   std::map<std::string, std::string> religion_of_tag_;
   std::map<std::string, std::string> group_of_religion_;
};

// What changes in one of EU5's country definitions.
struct Redefinition
{
   // A converted country that reuses a tag EU5 already defines takes its culture and religion from
   // that definition, so Makuria would start Sunni as EU5's 1337 has it rather than as the CK3 save does.
   std::optional<std::string> culture;
   std::optional<std::string> religion;
   // Whether the tag is one EU5 keeps only for history: true for those that don't exist in the
   // converted world, false for those a converted country brings back.
   std::optional<bool> historic;
};

// A country definition file with the given tags' definitions changed.
[[nodiscard]] std::string Redefine(const std::string& definitions,
    const std::map<std::string, Redefinition>& redefinitions);

}  // namespace eu5

#endif  // EU5_COUNTRY_SETUP_H
