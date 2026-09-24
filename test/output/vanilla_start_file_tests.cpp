#include <set>
#include <string>

#include "gtest/gtest.h"
#include "src/output/out_file_classes/setup/vanilla_start_file.hpp"

namespace out
{

TEST(OutputVanillaStartFileTests, EntriesAboutOnlyKeptCountriesStay)  // NOLINT : clang-tidy doens't like gtest
{
   const std::string rivals =
       "diplomacy_manager = {\n"
       "\trival = { first = FRA second = ENG }\n"
       "\trival = { first = AZT second = INC } # both kept\n"
       "\trival = { first = AZT second = ENG }\n"
       "}\n";

   const auto kept = KeepEntriesAbout(rivals, 1, {"AZT", "INC"});

   EXPECT_EQ(
       "diplomacy_manager = {\n"
       "\trival = { first = AZT second = INC } # both kept\n"
       "}\n",
       kept);
}

TEST(OutputVanillaStartFileTests, MultiLineEntriesAreJudgedWhole)  // NOLINT : clang-tidy doens't like gtest
{
   const std::string wars =
       "war_manager = {\n"
       "\tcivil_war = {\n"
       "\t\twar_name = { name = \"CIVIL_WAR_NAME\" first = { name = \"SCO\" } }\n"
       "\t\tattacker = { country = SBL }\n"
       "\t}\n"
       "\twar = {\n"
       "\t\tattacker = { country = AZT }\n"
       "\t\tdefender = { country = TLX } # TLX is not kept\n"
       "\t}\n"
       "\twar = {\n"
       "\t\tattacker = { country = AZT }\n"
       "\t\tdefender = { country = INC }\n"
       "\t}\n"
       "}\n";

   const auto kept = KeepEntriesAbout(wars, 1, {"AZT", "INC"});

   EXPECT_EQ(
       "war_manager = {\n"
       "\twar = {\n"
       "\t\tattacker = { country = AZT }\n"
       "\t\tdefender = { country = INC }\n"
       "\t}\n"
       "}\n",
       kept);
}

TEST(OutputVanillaStartFileTests, DeeperEntriesAndCommentsAreHandled)  // NOLINT : clang-tidy doens't like gtest
{
   const std::string personalities =
       "countries = {\n"
       "\tcountries = {\n"
       "\t\t# a comment naming FRA is not an entry\n"
       "\t\tFRA = { ai_personality = ai_expansionist }  # France\n"
       "\t\tAZT = { ai_personality = ai_aggressive }\n"
       "\t}\n"
       "}\n";

   const auto kept = KeepEntriesAbout(personalities, 2, {"AZT"});

   EXPECT_EQ(
       "countries = {\n"
       "\tcountries = {\n"
       "\t\t# a comment naming FRA is not an entry\n"
       "\t\tAZT = { ai_personality = ai_aggressive }\n"
       "\t}\n"
       "}\n",
       kept);
}

namespace
{
BuildingOwnership MakeOwnership()
{
   BuildingOwnership ownership;
   ownership.vanilla_owners = {{"stockholm", "SWE"}, {"visby", "SWE"}, {"bergen", "NOR"}, {"cuzco", "INC"},
       {"toulouse", "FRA"}, {"lyon", "FRA"}};
   ownership.current_owners = {{"stockholm", "DAN"}, {"bergen", "NOR"}, {"cuzco", "INC"}, {"toulouse", "TOU"},
       {"lyon", "FRA"}};
   ownership.kept_tags = {"INC"};
   ownership.religions = {{"DAN", "catholic"}, {"NOR", "catholic"}, {"TOU", "catharism"}, {"FRA", "catholic"}};
   return ownership;
}
}  // namespace

TEST(OutputVanillaStartFileTests, ABuildingPassesToTheLandsNewOwner)  // NOLINT : clang-tidy doens't like gtest
{
   const auto fitted = FitBuilding("\tcastle = { tag = SWE level = 1 location = stockholm } # the Three Crowns\n", MakeOwnership());

   ASSERT_TRUE(fitted.has_value());
   EXPECT_EQ("\tcastle = { tag = DAN level = 1 location = stockholm } # the Three Crowns\n", *fitted);
}

TEST(OutputVanillaStartFileTests, BuildingsOnLandNobodyHoldsGo)  // NOLINT : clang-tidy doens't like gtest
{
   EXPECT_FALSE(FitBuilding("\tcastle = { tag = SWE level = 1 location = visby }\n", MakeOwnership()).has_value());
}

TEST(OutputVanillaStartFileTests, ForeignOwnedBuildingsStayOnlyBetweenKeptCountries)  // NOLINT : clang-tidy doens't like gtest
{
   // A Swedish building in Norway describes a 1337 relationship the converted world doesn't have.
   EXPECT_FALSE(FitBuilding("\tkontor = { tag = SWE level = 1 location = bergen }\n", MakeOwnership()).has_value());
}

TEST(OutputVanillaStartFileTests, KeptCountriesKeepTheirBuildings)  // NOLINT : clang-tidy doens't like gtest
{
   const std::string temple = "\ttemple = { tag = INC level = 2 location = cuzco }\n";

   EXPECT_EQ(temple, FitBuilding(temple, MakeOwnership()));
}

TEST(OutputVanillaStartFileTests, CardinalsSitOnlyInCatholicLands)  // NOLINT : clang-tidy doens't like gtest
{
   EXPECT_FALSE(
       FitBuilding("\tseat_of_cardinal = { tag = FRA level = 1 location = toulouse }\n", MakeOwnership()).has_value());
   EXPECT_TRUE(
       FitBuilding("\tseat_of_cardinal = { tag = FRA level = 1 location = lyon }\n", MakeOwnership()).has_value());
}

TEST(OutputVanillaStartFileTests, EntriesWithoutAnOwnerAreKept)  // NOLINT : clang-tidy doens't like gtest
{
   const std::string location = "\tstockholm = { rank = city }\n";

   EXPECT_EQ(location, FitBuilding(location, MakeOwnership()));
}

}  // namespace out
