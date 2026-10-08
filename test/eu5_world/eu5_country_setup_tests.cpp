#include <map>
#include <string>
#include <utility>
#include <vector>

#include "gtest/gtest.h"
#include "src/eu5_world/eu5_country_setup.hpp"

namespace eu5
{

namespace
{
CountrySetup MakeSetup()
{
   CountrySetup setup;
   setup.AddTemplate("catholic_monarchy_not_present", "government = {\n\ttype = monarchy\n}\n");
   setup.AddTemplate("catholic_monarchy_english_lordship", "include = catholic_monarchy_not_present\n");
   setup.AddTemplate("catholic_monarchy", "government = { type = monarchy }\n");
   setup.AddTemplate("catholic_monarchy_no_coast", "government = { type = monarchy }\n");
   setup.AddTemplate("hansa", "government = { type = republic }\n");
   setup.AddTemplate("expl_western_europe", "discovered_regions = { france_region }\n");
   setup.AddDefinitionFile("france.txt",
       "\xEF\xBB\xBF#France - FRA\nFRA = {\n\tcolor = map_FRA\n\tculture_definition = french\n"
       "\treligion_definition = catholic\n}\n");
   setup.AddReligions(
       "catholic = {\n\tgroup = christian\n\tdefinition_modifier = { }\n}\nsunni = {\n\tgroup = muslim\n}\n");
   return setup;
}
}  // namespace

TEST(EU5WorldCountrySetupTests,
    GovernmentTypeComesFromTheBlockOrItsTemplates)  // NOLINT : clang-tidy doens't like gtest
{
   const auto setup = MakeSetup();

   EXPECT_EQ("republic", setup.GovernmentTypeOf("HSA = {\n\ttype = building\n\tgovernment = { type = republic }\n}\n"));
   EXPECT_EQ("monarchy",
       setup.GovernmentTypeOf("ENG = {\n\tinclude = \"catholic_monarchy_english_lordship\" # type = tribe\n}\n"));
   EXPECT_FALSE(setup.GovernmentTypeOf("SMI = {\n\ttype = pop\n}\n").has_value());
}

TEST(EU5WorldCountrySetupTests, GovernmentTemplatesLeaveExplorationOut)  // NOLINT : clang-tidy doens't like gtest
{
   const auto setup = MakeSetup();

   EXPECT_EQ(std::vector<std::string>{"catholic_monarchy"},
       setup.GovernmentTemplatesOf("FRA = {\n\tinclude = \"catholic_monarchy\"\n\tinclude = \"expl_western_europe\"\n"
                                   "\t# include = \"hansa\"\n\tinclude = \"nowhere\"\n}\n"));
   EXPECT_EQ("catholic_monarchy_no_coast", setup.LandlockedVariantOf("catholic_monarchy"));
   EXPECT_EQ("catholic_monarchy_no_coast", setup.LandlockedVariantOf("catholic_monarchy_no_coast"));
   EXPECT_EQ("hansa", setup.LandlockedVariantOf("hansa"));
}

TEST(EU5WorldCountrySetupTests, CountriesNotPresentTakeTheNotPresentTemplate)  // NOLINT : clang-tidy doens't like gtest
{
   const auto setup = MakeSetup();

   EXPECT_EQ("catholic_monarchy_not_present", setup.NotPresentVariantOf("catholic_monarchy"));
   EXPECT_EQ("catholic_monarchy_not_present", setup.NotPresentVariantOf("catholic_monarchy_no_coast"));
   EXPECT_EQ("catholic_monarchy_not_present", setup.NotPresentVariantOf("catholic_monarchy_not_present"));
   EXPECT_EQ("hansa", setup.NotPresentVariantOf("hansa"));
}

TEST(EU5WorldCountrySetupTests, DefinitionsAndReligionGroupsAreRead)  // NOLINT : clang-tidy doens't like gtest
{
   const auto setup = MakeSetup();

   EXPECT_EQ("french", setup.CultureOf("FRA"));
   EXPECT_EQ("catholic", setup.ReligionOf("FRA"));
   EXPECT_FALSE(setup.ReligionOf("ENG").has_value());
   EXPECT_EQ("christian", setup.GroupOf("catholic"));
   EXPECT_EQ("muslim", setup.GroupOf("sunni"));
   EXPECT_FALSE(setup.GroupOf("definition_modifier").has_value());
   ASSERT_EQ(1, setup.GetDefinitionFiles().size());
   EXPECT_EQ("france.txt", setup.GetDefinitionFiles().front().name);
}

TEST(EU5WorldCountrySetupTests,
    RedefinedTagsTakeTheConvertedCultureAndReligion)  // NOLINT : clang-tidy doens't like gtest
{
   const std::string definitions =
       "\xEF\xBB\xBF#Makuria - MAK\nMAK = {\n\tcolor = map_makuria\n\tculture_definition = nubian\n"
       "\treligion_definition = sunni\n}\n\n#Baqlin - BQL\nBQL = {\n\tculture_definition = beja_culture\n"
       "\treligion_definition = sunni\n}\n";

   EXPECT_EQ(
       "\xEF\xBB\xBF#Makuria - MAK\nMAK = {\n\tcolor = map_makuria\n\tculture_definition = nubian\n"
       "\treligion_definition = coptic\n}\n\n#Baqlin - BQL\nBQL = {\n\tculture_definition = beja_culture\n"
       "\treligion_definition = sunni\n}\n",
       Redefine(definitions, {{"MAK", {.religion = "coptic"}}}));
   EXPECT_EQ(definitions, Redefine(definitions, {}));
}

TEST(EU5WorldCountrySetupTests, TagsThatDontExistAreMarkedHistoric)  // NOLINT : clang-tidy doens't like gtest
{
   const std::string definitions =
       "DAN = { # Denmark\n\tcolor = map_DAN\n}\nCRI = {\n\tis_historic = yes\t#Released during the Crisis\n"
       "\tcolor = rgb { 1 2 3 }\n}\nSWE = {\n\tcolor = map_SWE\n}";

   EXPECT_EQ(
       "DAN = { # Denmark\n\tis_historic = yes\n\tcolor = map_DAN\n}\nCRI = {\n\tcolor = rgb { 1 2 3 }\n}\n"
       "SWE = {\n\tcolor = map_SWE\n}",
       Redefine(definitions, {{"DAN", {.historic = true}}, {"CRI", {.historic = false}}}));
}

}  // namespace eu5
