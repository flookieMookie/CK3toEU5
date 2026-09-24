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

}  // namespace eu5
