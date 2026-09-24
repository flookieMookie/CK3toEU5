#include "gtest/gtest.h"
#include "src/eu5_world/eu5_country.hpp"

namespace eu5
{

TEST(EU5WorldCountryTests, CharacterNameKeysAreSharedByName)  // NOLINT : clang-tidy doens't like gtest
{
   EXPECT_EQ("ck3_name_louis", CharacterNameKey("Louis"));
   EXPECT_EQ(CharacterNameKey("Louis"), CharacterNameKey("louis"));
   EXPECT_EQ("ck3_name_al_mu_tazz", CharacterNameKey("Al-Mu'tazz"));
}

TEST(EU5WorldCountryTests, NamesWithNothingUsableHaveNoKey)  // NOLINT : clang-tidy doens't like gtest
{
   EXPECT_TRUE(CharacterNameKey("").empty());
   EXPECT_TRUE(CharacterNameKey("--").empty());
}

TEST(EU5WorldCountryTests, StartingGoldCarriesOverWithinEU5sRange)  // NOLINT : clang-tidy doens't like gtest
{
   EXPECT_EQ(452, StartingGoldFor(452.03832));
   EXPECT_EQ(0, StartingGoldFor(0.2));
   EXPECT_EQ(2500, StartingGoldFor(40000.0));
   EXPECT_EQ(-500, StartingGoldFor(-9000.0));
   EXPECT_EQ(-120, StartingGoldFor(-120.4));
}

TEST(EU5WorldCountryTests, SkillsBecomeAbilitiesOnEU5sScale)  // NOLINT : clang-tidy doens't like gtest
{
   EXPECT_EQ(15, AbilityFromSkill(0));
   EXPECT_EQ(45, AbilityFromSkill(5));  // CK3's average ruler, EU5's average character
   EXPECT_EQ(75, AbilityFromSkill(10));
   EXPECT_EQ(100, AbilityFromSkill(30));
   EXPECT_EQ(0, AbilityFromSkill(-5));
}

TEST(EU5WorldCountryTests, EscapedNamesAreRestored)  // NOLINT : clang-tidy doens't like gtest
{
   EXPECT_EQ("Cui\xE6\xBC\xBC", CleanCK3Name("Cui_6F3C"));
   EXPECT_EQ("Ethelred", CleanCK3Name("E_thelred"));
   EXPECT_EQ("Sigfrit", CleanCK3Name("SigfriT_"));
}

TEST(EU5WorldCountryTests, WordsAfterAnUnderscoreAreNotEscapes)  // NOLINT : clang-tidy doens't like gtest
{
   // _Daba would be half a surrogate pair, which as UTF-8 made the whole localisation file invalid.
   EXPECT_EQ("Domnall Dabaill", CleanCK3Name("Domnall_Dabaill"));
   EXPECT_EQ("Abu Abdallah", CleanCK3Name("Abu_Abdallah"));
   EXPECT_EQ("Aillil Fland Becc", CleanCK3Name("Aillil_Fland_Becc"));
}

}  // namespace eu5
