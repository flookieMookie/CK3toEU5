#ifndef EU5_VANILLA_CHARACTERS_H
#define EU5_VANILLA_CHARACTERS_H

#include <filesystem>
#include <istream>
#include <string>
#include <vector>

namespace eu5
{

// A character exactly as EU5 writes it in its own 05_characters.txt, kept as raw text.
struct VanillaCharacter
{
   std::string id;
   std::string tag;    // the country the character belongs to
   std::string block;  // the whole id = { ... } block, verbatim
};

// EU5's own starting characters.
//
// The converter replaces 05_characters.txt, and the vanilla countries it keeps on land CK3 doesn't
// cover are carried over verbatim - naming rulers, heirs and regents from this file. Without their
// characters those countries point at people who don't exist, so they come along too.
class VanillaCharacters
{
  public:
   VanillaCharacters() = default;
   explicit VanillaCharacters(const std::filesystem::path& eu5_directory);
   // For tests: characters from an already open file.
   explicit VanillaCharacters(std::istream& input_stream);

   [[nodiscard]] const auto& GetCharacters() const { return characters_; }

  private:
   void Parse(std::istream& input_stream);

   std::vector<VanillaCharacter> characters_;
};

}  // namespace eu5

#endif  // EU5_VANILLA_CHARACTERS_H
