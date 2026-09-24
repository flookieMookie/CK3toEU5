#include <string>

#include "gtest/gtest.h"
#include "src/output/out_file_classes/setup/international_organizations_file.hpp"

namespace out
{

namespace
{
OrganizationMembership MakeMembership()
{
   OrganizationMembership membership;
   membership.kept_tags = {"AZT", "INC"};
   membership.converted_religions = {{"PAP", "catholic"}, {"BYZ", "orthodox"}, {"SER", "catholic"}, {"HAB", "catholic"}};
   membership.characters = {"azt_tenoch"};
   return membership;
}
}  // namespace

TEST(OutputInternationalOrganizationsTests, AChurchKeepsAConvertedLeaderOfItsFaith)  // NOLINT : clang-tidy doens't like gtest
{
   const std::string church =
       "\tadd_international_organization = {\n"
       "\t\ttype = catholic_church\n"
       "\t\tmembers = { PAP }\n"
       "\t\tleader = PAP\n"
       "\t\truler_term = { character = pap_benedetto_xii start_date = 1334.12.20 }\n"
       "\t}\n";

   const auto fitted = FitOrganization(church, MakeMembership());

   ASSERT_TRUE(fitted.has_value());
   EXPECT_EQ(
       "\tadd_international_organization = {\n"
       "\t\ttype = catholic_church\n"
       "\t\tmembers = { PAP }\n"
       "\t\tleader = PAP\n"
       "\t}\n",
       *fitted);
}

TEST(OutputInternationalOrganizationsTests, MembersNoLongerOfTheFaithLeave)  // NOLINT : clang-tidy doens't like gtest
{
   const std::string patriarchate =
       "\tadd_international_organization = {\n"
       "\t\ttype = autocephalous_patriarchate\n"
       "\t\tmembers = {\n"
       "\t\t\tBYZ SER NOV # Serbia converted Catholic; Novgorod was replaced\n"
       "\t\t}\n"
       "\t\tvariables = { religion = religion:orthodox seat = location:constantinople }\n"
       "\t}\n";

   const auto fitted = FitOrganization(patriarchate, MakeMembership());

   ASSERT_TRUE(fitted.has_value());
   EXPECT_NE(std::string::npos, fitted->find("members = { BYZ }"));
   EXPECT_NE(std::string::npos, fitted->find("seat = location:constantinople"));
}

TEST(OutputInternationalOrganizationsTests, PoliticalOnesNeedTheirLeaderKept)  // NOLINT : clang-tidy doens't like gtest
{
   const std::string hre =
       "\tadd_international_organization = {\n"
       "\t\ttype = hre\n"
       "\t\tmembers = { HAB AZT }\n"
       "\t\tleader = HAB\n"
       "\t}\n";

   // HAB converted, and a converted country keeps a political membership by tag alone never.
   EXPECT_FALSE(FitOrganization(hre, MakeMembership()).has_value());
}

TEST(OutputInternationalOrganizationsTests, EmptyOrganizationsGo)  // NOLINT : clang-tidy doens't like gtest
{
   const std::string sect =
       "\tadd_international_organization = {\n"
       "\t\ttype = sect\n"
       "\t\tmembers = { DAI SSG }\n"
       "\t\tvariables = { religion = religion:mahayana }\n"
       "\t}\n";

   EXPECT_FALSE(FitOrganization(sect, MakeMembership()).has_value());
}

}  // namespace out
