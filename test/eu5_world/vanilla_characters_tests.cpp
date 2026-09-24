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


TEST(EU5WorldVanillaCharactersTests, KeptCountriesAdoptTheForeignRulersTheyName)  // NOLINT : clang-tidy doens't like gtest
{
   std::stringstream input;
   input << "character_db = {\n";
   input << "\tgrl_bishop = {\n\t\ttag = GRL\n\t}\n";
   input << "\tswe_erik = {\n\t\ttag = SWE\n\t}\n";
   input << "\tswe_magnus = {\n";
   input << "\t\tfather = swe_erik\n";
   input << "\t\tspouse = grl_bishop # not really\n";
   input << "\t\ttag = SWE\n";
   input << "\t}\n";
   input << "}\n";
   const VanillaCharacters characters(input);
   const VanillaCountry greenland{.tag = "GRL",
       .block = "GRL = { # Greenland\n\tgovernment = {\n\t\truler = swe_magnus # Magnus of Sweden\n\t}\n\t# heir = swe_erik\n}\n",
       .locations = {}};

   const auto kept = characters.KeptFor({&greenland});

   ASSERT_EQ(2, kept.size());
   EXPECT_EQ("grl_bishop", kept[0].id);
   EXPECT_EQ("swe_magnus", kept[1].id);
   EXPECT_EQ("GRL", kept[1].tag);
   // Sweden's Erik stays behind, so Magnus loses his father but keeps his Greenlandic spouse.
   EXPECT_EQ("\tswe_magnus = {\n\t\tspouse = grl_bishop # not really\n\t\ttag = GRL\n\t}\n", kept[1].block);
}

}  // namespace eu5
