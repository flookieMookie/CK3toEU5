#include <sstream>

#include "gtest/gtest.h"
#include "src/eu5_world/eu5_vanilla_characters.hpp"

namespace eu5
{

TEST(EU5WorldVanillaCharactersTests, CharactersAreReadWithTheirTags)  // NOLINT : clang-tidy doens't like gtest
{
   std::stringstream input;
   input << "\xEF\xBB\xBF# EU5's own characters\n";
   input << "character_db = {\n";
   input << "\tazt_tenoch = {\n";
   input << "\t\tfirst_name = { name = name_tenoch }\n";
   input << "\t\tculture = mexica_culture\n";
   input << "\t\ttag = AZT\n";
   input << "\t}\n";
   input << "\tnor_agnes = { # a comment with a { brace\n";
   input << "\t\tfirst_name = { name = name_agnes }\n";
   input << "\t\tfather = nor_hakan_magnusson\n";
   input << "\t\ttag = NOR\n";
   input << "\t}\n";
   input << "}\n";
   const VanillaCharacters characters(input);

   ASSERT_EQ(2, characters.GetCharacters().size());
   EXPECT_EQ("azt_tenoch", characters.GetCharacters()[0].id);
   EXPECT_EQ("AZT", characters.GetCharacters()[0].tag);
   EXPECT_EQ("nor_agnes", characters.GetCharacters()[1].id);
   EXPECT_EQ("NOR", characters.GetCharacters()[1].tag);
}

TEST(EU5WorldVanillaCharactersTests, BlocksAreKeptVerbatim)  // NOLINT : clang-tidy doens't like gtest
{
   std::stringstream input;
   input << "character_db = {\n";
   input << "\tazt_tenoch = {\n";
   input << "\t\tfirst_name = { name = name_tenoch }\n";
   input << "\t\ttag = AZT\n";
   input << "\t}\n";
   input << "}\n";
   const VanillaCharacters characters(input);

   ASSERT_EQ(1, characters.GetCharacters().size());
   EXPECT_EQ("\tazt_tenoch = {\n\t\tfirst_name = { name = name_tenoch }\n\t\ttag = AZT\n\t}\n",
       characters.GetCharacters()[0].block);
}

}  // namespace eu5
