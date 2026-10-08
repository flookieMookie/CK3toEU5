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
   EXPECT_EQ("cognatic_primogeniture",
       HeirSelectionFor("monarchy", {"saxon_elective_succession_law", "male_preference_law"}));
   EXPECT_EQ("absolute_cognatic_primogeniture", HeirSelectionFor("monarchy", {"equal_law"}));
   EXPECT_EQ("absolute_cognatic_primogeniture", HeirSelectionFor("monarchy", {"female_only_law"}));
   EXPECT_EQ("cognatic_primogeniture", HeirSelectionFor("monarchy", {}));
}

TEST(OutputCountriesFileTests, OtherGovernmentsTakeWhatEU5sOwnUse)  // NOLINT : clang-tidy doens't like gtest
{
   EXPECT_EQ("tribal_oldest_male", HeirSelectionFor("tribe", {"confederate_partition_succession_law"}));
   EXPECT_EQ("theocratic_elective", HeirSelectionFor("theocracy", {"bishop_theocratic_succession_law"}));
   EXPECT_EQ("oligarchic_elective", HeirSelectionFor("republic", {"city_succession_law"}));
}

TEST(OutputCountriesFileTests, MarriageFollowsGovernmentAndFaith)  // NOLINT : clang-tidy doens't like gtest
{
   EXPECT_EQ("celibacy", MarriageLawFor("theocracy", "muslim"));
   EXPECT_EQ("muslim_marriage", MarriageLawFor("monarchy", "muslim"));
   EXPECT_EQ("monogamous_marriage", MarriageLawFor("tribe", "christian"));
   EXPECT_EQ("monogamous_marriage", MarriageLawFor("monarchy", std::nullopt));
}

TEST(OutputCountriesFileTests, TheocraciesChooseHeirsFromTheirClergy)  // NOLINT : clang-tidy doens't like gtest
{
   EXPECT_EQ("heir_special_succession", HeirReligionLawFor("theocratic_elective"));
   EXPECT_EQ("heir_same_religion", HeirReligionLawFor("partition_inheritance"));
}

TEST(OutputCountriesFileTests, CountriesNotPresentKeepNoLandOrRulers)  // NOLINT : clang-tidy doens't like gtest
{
   const std::string denmark =
       "\tDAN = {\n"
       "\t\town_control_core = {\n"
       "\t\t\troskilde kobenhavn # Zealand\n"
       "\t\t}\n"
       "\t\tour_cores_conquered_by_others = { lund malmo }\n"
       "\t\tcapital = roskilde\n"
       "\t\tinclude = \"expl_scandinavia\"\n"
       "\t\tinclude = \"catholic_monarchy_no_coast\"\n"
       "\t\tgovernment = {\n"
       "\t\t\ttype = monarchy\n"
       "\t\t\truler_term = { character = dan_eric start_date = 1286.1.1 end_date = 1319.1.1 }\n"
       "\t\t\truler_term = {\n"
       "\t\t\t\tcharacter = dan_christopher # { not a brace }\n"
       "\t\t\t}\n"
       "\t\t\truler = dan_christopher\n"
       "\t\t\their = random\n"
       "\t\t\tconsort = dan_euphemia\n"
       "\t\t\tactive_regent = shl_gerhard\n"
       "\t\t}\n"
       "\t}\n";

   eu5::CountrySetup setup;
   setup.AddTemplate("catholic_monarchy", "");
   setup.AddTemplate("catholic_monarchy_no_coast", "");
   setup.AddTemplate("catholic_monarchy_not_present", "");

   EXPECT_EQ(
       "\tDAN = {\n"
       "\t\tcapital = roskilde\n"
       "\t\tinclude = \"expl_scandinavia\"\n"
       "\t\tinclude = \"catholic_monarchy_not_present\"\n"
       "\t\tgovernment = {\n"
       "\t\t\ttype = monarchy\n"
       "\t\t\their = random\n"
       "\t\t}\n"
       "\t}\n",
       NotPresentBlock(denmark, eu5::MapAreas(), setup));
}

TEST(OutputCountriesFileTests, PopCountriesGatherNoPopsFromConvertedLand)  // NOLINT : clang-tidy doens't like gtest
{
   std::stringstream definitions;
   definitions
       << "europe = { eastern_europe = { russia_region = { kola_area = { kola_province = { kola alta } } } } }\n";
   const eu5::MapAreas map_areas(definitions);

   // With no capital named, the first place it gathered pops from stands in.
   EXPECT_EQ("\tSMI = {\n\t\tcapital = kola\n\t\tdiscovered_regions = { russia_region }\n\t\ttype = pop\n\t}\n",
       NotPresentBlock("\tSMI = {\n\t\ttype = pop\n\t\tadd_pops_from_locations = {\n\t\t\tkola alta\n\t\t}\n\t}\n",
           map_areas,
           eu5::CountrySetup()));
}

TEST(OutputCountriesFileTests,
    ConvertedCountriesKnowTheWorldTheirLandsOwnerKnew)  // NOLINT : clang-tidy doens't like gtest
{
   std::stringstream definitions;
   definitions
       << "europe = { western_europe = { france_region = { ile_de_france_area = { paris_province = { paris } } }\n";
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

TEST(OutputCountriesFileTests, MuslimCountriesNameASchoolTheyMayHold)  // NOLINT : clang-tidy doens't like gtest
{
   EXPECT_EQ("maturidi_school", ReligiousSchoolFor("sunni"));
   EXPECT_EQ("ismaili_school", ReligiousSchoolFor("shia"));
   EXPECT_EQ("ibadi_school", ReligiousSchoolFor("ibadi"));
   // EU5's Hindu and Jain schools aren't ones a country names; only its Muslim religions list them.
   EXPECT_FALSE(ReligiousSchoolFor("hindu").has_value());
   EXPECT_FALSE(ReligiousSchoolFor("catholic").has_value());
   EXPECT_FALSE(ReligiousSchoolFor(std::nullopt).has_value());
}

}  // namespace out
