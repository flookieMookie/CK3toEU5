#include <sstream>

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

TEST(EU5WorldCountryTests, NicknamesAreKeyedByTheirText)  // NOLINT : clang-tidy doens't like gtest
{
   EXPECT_EQ("ck3_nick_the_great", NicknameKey("nick_the_great", "the Great"));
   // Two characters with CK3's culture nickname read differently, so they get different keys.
   EXPECT_EQ("ck3_nick_the_irish", NicknameKey("nick_the_own_culture", "the Irish"));
   EXPECT_TRUE(NicknameKey("", "").empty());
}

TEST(EU5WorldCountryTests, NicknamesUseCK3sTextWhereItIsPlain)  // NOLINT : clang-tidy doens't like gtest
{
   commonItems::LocalizationDatabase nicknames("english", {"french", "german"});
   std::stringstream english;
   english
       << "l_english:\n nick_the_great: \"the Great\"\n nick_the_bald: \"the Bald\"\n"
       << " nick_the_bald_ironic: \"$nick_the_bald$\"\n nick_the_own_culture: \"the [CHARACTER.GetCulture.GetName]\"\n";
   std::stringstream french;
   french << "l_french:\n nick_the_great: \"[CHARACTER.Custom('FR_LeLa')] Grand\"\n";
   std::stringstream german;
   german << "l_german:\n nick_the_great: \"der Gro\xC3\x9F"
             "e\"\n";
   ASSERT_GT(nicknames.ScrapeStream(english), 0);
   ASSERT_GT(nicknames.ScrapeStream(french), 0);
   ASSERT_GT(nicknames.ScrapeStream(german), 0);

   EXPECT_EQ("the Great", NicknameText("nick_the_great", "the Great", nicknames, "english"));
   EXPECT_EQ(
       "der Gro\xC3\x9F"
       "e",
       NicknameText("nick_the_great", "the Great", nicknames, "german"));
   // Scripted by gender in French, so the save's own text stands in.
   EXPECT_EQ("the Great", NicknameText("nick_the_great", "the Great", nicknames, "french"));
   EXPECT_EQ("the Bald", NicknameText("nick_the_bald_ironic", "the Bald", nicknames, "english"));
   EXPECT_EQ("the Irish", NicknameText("nick_the_own_culture", "the Irish", nicknames, "english"));
}

TEST(EU5WorldCountryTests, ACountryFollowsItsRulersFaithNotItsLargest)  // NOLINT : clang-tidy doens't like gtest
{
   // An Orthodox emperor who conquered China: most of his subjects are Buddhist, the empire is not.
   const PopulationShares population = {{"mahayana", 9000.0}, {"orthodox", 800.0}, {"sunni", 300.0}};
   EXPECT_EQ("orthodox", ChooseStateReligion("orthodox", population));
}

TEST(EU5WorldCountryTests, ARulersFaithStandsEvenWhereNobodySharesIt)  // NOLINT : clang-tidy doens't like gtest
{
   // A Norse king of York, whose people are all Catholic.
   const PopulationShares population = {{"catholic", 900.0}, {"judaism", 10.0}};
   EXPECT_EQ("norse", ChooseStateReligion("norse", population));
}

TEST(EU5WorldCountryTests, AFaithEU5HasNoReligionForLeavesItToThePeople)  // NOLINT : clang-tidy doens't like gtest
{
   const PopulationShares population = {{"mahayana", 9000.0}, {"sunni", 300.0}, {"orthodox", 0.0}};
   EXPECT_EQ("mahayana", ChooseStateReligion(std::nullopt, population));
}

TEST(EU5WorldCountryTests, ACountryWithoutPeopleChoosesNothing)  // NOLINT : clang-tidy doens't like gtest
{
   EXPECT_FALSE(ChooseStateReligion(std::nullopt, {}).has_value());
   EXPECT_FALSE(ChoosePrimaryCulture(
       [](const std::string&) {
          return true;
       },
       {})
           .has_value());
}

TEST(EU5WorldCountryTests, ACountryHasTheLargestOfItsRulersCultures)  // NOLINT : clang-tidy doens't like gtest
{
   // CK3's Greek covers several of EU5's cultures; the largest of them in the country is its own.
   const PopulationShares population = {{"han", 9000.0}, {"greek", 500.0}, {"pontic", 700.0}, {"turkish", 600.0}};
   const auto greek = [](const std::string& culture) {
      return culture == "greek" || culture == "pontic";
   };
   EXPECT_EQ("pontic", ChoosePrimaryCulture(greek, population));
}

TEST(EU5WorldCountryTests, ARulerWithNoneOfTheirOwnPeopleHasThePeoplesCulture)  // NOLINT
{
   const PopulationShares population = {{"han", 9000.0}, {"turkish", 600.0}};
   const auto norse = [](const std::string& culture) {
      return culture == "norse";
   };
   EXPECT_EQ("han", ChoosePrimaryCulture(norse, population));
}

TEST(EU5WorldCountryTests, EqualSharesGoToTheFirstByName)  // NOLINT : clang-tidy doens't like gtest
{
   const PopulationShares population = {{"sunni", 50.0}, {"hindu", 50.0}};
   EXPECT_EQ("hindu", ChooseStateReligion(std::nullopt, population));
}

}  // namespace eu5
