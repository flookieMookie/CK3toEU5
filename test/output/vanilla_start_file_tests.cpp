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

}  // namespace out
