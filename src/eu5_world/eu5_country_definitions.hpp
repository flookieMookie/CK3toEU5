#ifndef EU5_COUNTRY_DEFINITIONS_H
#define EU5_COUNTRY_DEFINITIONS_H

#include <filesystem>
#include <set>
#include <string>

namespace eu5
{

// The country tags EU5 actually defines, scraped from the game's own
// in_game/setup/countries/*.txt. Writing a tag EU5 does not know about into 10_countries.txt makes
// the game reject the block, and an early one takes the rest of the file down with it, so every
// converted country is checked against this before it is written.
class CountryDefinitions
{
  public:
   CountryDefinitions() = default;
   explicit CountryDefinitions(const std::filesystem::path& eu5_directory);

   [[nodiscard]] bool Contains(const std::string& tag) const { return tags_.contains(tag); }
   [[nodiscard]] const auto& GetTags() const { return tags_; }

  private:
   void LoadFile(const std::filesystem::path& file_path);

   std::set<std::string> tags_;
};

}  // namespace eu5

#endif  // EU5_COUNTRY_DEFINITIONS_H
