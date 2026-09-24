#include <set>
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

}  // namespace out
