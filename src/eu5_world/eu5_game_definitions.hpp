#ifndef EU5_GAME_DEFINITIONS_H
#define EU5_GAME_DEFINITIONS_H

#include <filesystem>
#include <set>
#include <string>

namespace eu5
{

// What EU5 actually defines: country tags, cultures and religions, scraped from the install.
//
// The configurables carry a lot of EU4 era names that EU5 renamed or never had - the Ottomans are
// TUR rather than OTT, Shia is shia rather than shiite - and writing one into the output makes EU5
// reject the block it appears in. A rejected block near the top of 10_countries.txt takes the rest
// of the file with it, silently, so everything the converter emits is checked against this first.
class GameDefinitions
{
  public:
   GameDefinitions() = default;
   explicit GameDefinitions(const std::filesystem::path& eu5_directory);

   [[nodiscard]] bool HasTag(const std::string& tag) const { return tags_.contains(tag); }
   [[nodiscard]] bool HasCulture(const std::string& culture) const { return cultures_.contains(culture); }
   [[nodiscard]] bool HasReligion(const std::string& religion) const { return religions_.contains(religion); }

   [[nodiscard]] const auto& GetTags() const { return tags_; }
   [[nodiscard]] const auto& GetCultures() const { return cultures_; }
   [[nodiscard]] const auto& GetReligions() const { return religions_; }

   // False when the EU5 install could not be read, in which case nothing can be validated.
   [[nodiscard]] bool IsLoaded() const { return !tags_.empty(); }

  private:
   // Collects the top level keys of every .txt in the folder.
   void LoadKeys(const std::filesystem::path& folder, std::set<std::string>& target, const std::string& pattern);

   std::set<std::string> tags_;
   std::set<std::string> cultures_;
   std::set<std::string> religions_;
};

}  // namespace eu5

#endif  // EU5_GAME_DEFINITIONS_H
