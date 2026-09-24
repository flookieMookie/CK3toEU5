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

}  // namespace eu5
