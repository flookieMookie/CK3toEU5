#include <string>
#include <vector>

#include "gtest/gtest.h"
#include "src/eu5_world/eu5_ruler_traits.hpp"

namespace eu5
{

TEST(EU5WorldRulerTraitsTests, PersonalityComesFirst)  // NOLINT : clang-tidy doens't like gtest
{
   const Abilities able{.adm = 60, .dip = 60, .mil = 60};

   EXPECT_EQ((std::vector<std::string>{"bold_fighter", "just", "zealot"}),
       RulerTraitsFor({"strategist", "zealous", "brave", "just", "education_martial_3"}, able));
}

TEST(EU5WorldRulerTraitsTests, TraitsNeedTheAbilitiesEU5AsksFor)  // NOLINT : clang-tidy doens't like gtest
{
   const Abilities weak{.adm = 30, .dip = 45, .mil = 40};

   // just needs adm over 33, a tactical genius mil of 50 or more; calm asks nothing.
   EXPECT_EQ((std::vector<std::string>{"calm"}), RulerTraitsFor({"just", "strategist", "calm"}, weak));
}

TEST(EU5WorldRulerTraitsTests, TraitsThatRuleEachOtherOutAreNotBothGiven)  // NOLINT : clang-tidy doens't like gtest
{
   const Abilities able{.adm = 60, .dip = 60, .mil = 60};

   // A patient man is careful, which EU5 won't let a bold fighter be.
   EXPECT_EQ((std::vector<std::string>{"bold_fighter"}), RulerTraitsFor({"brave", "patient"}, able));
   // A zealous drunkard keeps his zeal.
   EXPECT_EQ((std::vector<std::string>{"zealot"}), RulerTraitsFor({"zealous", "drunkard"}, able));
}

TEST(EU5WorldRulerTraitsTests, UnmappedTraitsGiveNothing)  // NOLINT : clang-tidy doens't like gtest
{
   EXPECT_TRUE(RulerTraitsFor({"lustful", "education_diplomacy_2", "beautiful_1"}, Abilities{}).empty());
}

TEST(EU5WorldRulerTraitsTests, TraitsComeWithYearsOnTheThrone)  // NOLINT : clang-tidy doens't like gtest
{
   // A reign begun in 856 on an 867 save began in 1326 on EU5's calendar: eleven years by 1337.
   const auto reign_start = ReignStart(date("856.3.1"), date("867.1.1"));
   ASSERT_TRUE(reign_start.has_value());
   EXPECT_EQ(date("1326.3.1"), *reign_start);
   EXPECT_EQ(2, RulerTraitSlots(reign_start));
   EXPECT_EQ(0, RulerTraitSlots(ReignStart(date("866.12.1"), date("867.1.1"))));
   EXPECT_EQ(3, RulerTraitSlots(ReignStart(date("830.1.1"), date("867.1.1"))));
   EXPECT_EQ(0, RulerTraitSlots(ReignStart(date("9999.1.1"), date("867.1.1"))));

   const Abilities able{.adm = 60, .dip = 60, .mil = 60};
   EXPECT_EQ((std::vector<std::string>{"bold_fighter"}), RulerTraitsFor({"brave", "just", "zealous"}, able, 1));
}

}  // namespace eu5
