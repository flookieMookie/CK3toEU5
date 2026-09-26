#include <set>
#include <sstream>
#include <string>

#include "gtest/gtest.h"
#include "src/output/out_file_classes/setup/countries_file.hpp"

namespace out
{

TEST(OutputCountriesFileTests, PartitionOutranksTheGenderLaw)  // NOLINT : clang-tidy doens't like gtest
{
   EXPECT_EQ("partition_inheritance",
       HeirSelectionFor("monarchy", {"confederate_partition_succession_law", "male_preference_law"}));
   EXPECT_EQ("partition_inheritance",
       HeirSelectionFor("monarchy", {"clan_impassive_partition_succession_law", "male_only_law"}));
}

TEST(OutputCountriesFileTests, TheGenderLawPicksThePrimogeniture)  // NOLINT : clang-tidy doens't like gtest
{
   EXPECT_EQ("salic_law", HeirSelectionFor("monarchy", {"single_heir_succession_law", "male_only_law"}));
   EXPECT_EQ("cognatic_primogeniture", HeirSelectionFor("monarchy", {"saxon_elective_succession_law", "male_preference_law"}));
   EXPECT_EQ("absolute_cognatic_primogeniture", HeirSelectionFor("monarchy", {"equal_law"}));
   EXPECT_EQ("absolute_cognatic_primogeniture", HeirSelectionFor("monarchy", {"female_only_law"}));
   EXPECT_EQ("cognatic_primogeniture", HeirSelectionFor("monarchy", {}));
}

TEST(OutputCountriesFileTests, OtherGovernmentsKeepEU5sDefault)  // NOLINT : clang-tidy doens't like gtest
{
   EXPECT_FALSE(HeirSelectionFor("tribe", {"confederate_partition_succession_law"}).has_value());
   EXPECT_FALSE(HeirSelectionFor("theocracy", {"bishop_theocratic_succession_law"}).has_value());
   EXPECT_FALSE(HeirSelectionFor("republic", {"city_succession_law"}).has_value());
}

TEST(OutputCountriesFileTests, ConvertedCountriesKnowTheWorldTheirLandsOwnerKnew)  // NOLINT : clang-tidy doens't like gtest
{
   std::stringstream definitions;
   definitions << "europe = { western_europe = { france_region = { ile_de_france_area = { paris_province = { paris } } }\n";
   definitions << "   italy_region = { lazio_area = { roma_province = { rome } } } } }\n";
   const eu5::MapAreas map_areas(definitions);
   const std::string vanilla_france =
       "\tFRA = {\n\t\tinclude = \"catholic_monarchy\"\n\t\tinclude = \"expl_northern_europe\" # the west\n"
       "\t\t# include = \"expl_china\"\n\t\tinclude = \"expl_mediterranean\"\n\t}\n";

   EXPECT_EQ(
       "\t\t\tinclude = \"expl_northern_europe\"\n"
       "\t\t\tinclude = \"expl_mediterranean\"\n"
       "\t\t\tdiscovered_regions = { france_region italy_region }\n",
       WriteDiscoveries({"paris", "rome", "nowhere"}, vanilla_france, map_areas));
}

TEST(OutputCountriesFileTests, LandUnownedIn1337StillKnowsItsOwnRegions)  // NOLINT : clang-tidy doens't like gtest
{
   std::stringstream definitions;
   definitions << "africa = { sahel = { sahel_region = { ghana_area = { kumbi_province = { kumbi_saleh } } } } }\n";
   const eu5::MapAreas map_areas(definitions);

   EXPECT_EQ("\t\t\tdiscovered_regions = { sahel_region }\n", WriteDiscoveries({"kumbi_saleh"}, "", map_areas));
}

TEST(OutputCountriesFileTests, TheCrownsAuthorityDecidesCentralization)  // NOLINT : clang-tidy doens't like gtest
{
   EXPECT_EQ(60, CentralizationFor("monarchy", {"crown_authority_0", "male_preference_law"}));
   EXPECT_EQ(0, CentralizationFor("monarchy", {"crown_authority_3"}));
   EXPECT_EQ(80, CentralizationFor("tribe", {"tribal_authority_0"}));
   EXPECT_EQ(20, CentralizationFor("tribe", {"tribal_authority_3"}));
   EXPECT_EQ(-20, CentralizationFor("republic", {"city_succession_law"}));
   EXPECT_EQ(40, CentralizationFor("tribe", {}));
}

}  // namespace out
