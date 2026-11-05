#include <sstream>
#include <stdexcept>

#include "gtest/gtest.h"
#include "src/ck3_world/cultures/cultures.hpp"
#include "src/ck3_world/geography/county_detail.hpp"
#include "src/ck3_world/geography/county_details.hpp"
#include "src/ck3_world/religions/religions.hpp"

TEST(CK3WorldCountyDetailsTests, DetailsDefaultToEmpty)  // NOLINT : clang-tidy doens't like gtest
{
   std::stringstream input;
   const ck3::CountyDetails details(input);

   ASSERT_TRUE(details.GetCountyDetails().empty());
}

TEST(CK3WorldCountyDetailsTests, DetailsCanBeLoadedIfPresent)  // NOLINT : clang-tidy doens't like gtest
{
   std::stringstream input;
   input << "counties = {\n";
   input << "c_county1 = {}\n";
   input << "c_county2 = { development = 8 culture = 3 faith = 9}\n";
   input << "c_county3 = { faith = 4 }\n";
   input << "}";

   const ck3::CountyDetails details(input);
   const auto& county1 = details.GetCountyDetails().find("c_county1");
   const auto& county2 = details.GetCountyDetails().find("c_county2");
   const auto& county3 = details.GetCountyDetails().find("c_county3");

   ASSERT_EQ(3, details.GetCountyDetails().size());
   ASSERT_EQ(0, county1->second->GetDevelopment());
   ASSERT_EQ(8, county2->second->GetDevelopment());
   ASSERT_EQ(9, county2->second->GetFaith().GetID());
   ASSERT_EQ(3, county2->second->GetCulture().GetID());
   ASSERT_EQ(4, county3->second->GetFaith().GetID());
}

TEST(CK3WorldCountyDetailsTests, FaithsCanBeLinked)  // NOLINT : clang-tidy doens't like gtest
{
   std::stringstream input;
   input << "counties = {\n";
   input << "c_county2 = { development = 8 culture = 3 faith = 9}\n";
   input << "c_county3 = { culture = 3 faith = 4 }\n";
   input << "}";
   ck3::CountyDetails details(input);

   std::stringstream input2;
   input2 << "faiths={\n";
   input2 << "9={tag=\"old_bon\"}\n";
   input2 << "4={tag=\"theravada\"}\n";
   input2 << "}";
   const ck3::Religions religions(input2);

   details.LinkReligions(religions);
   const auto& county2 = details.GetCountyDetails().find("c_county2");
   const auto& county3 = details.GetCountyDetails().find("c_county3");

   ASSERT_EQ("old_bon", county2->second->GetFaith().GetPointer().lock()->GetTag());
   ASSERT_EQ("theravada", county3->second->GetFaith().GetPointer().lock()->GetTag());
}

// Since CK3 1.20 a county names its rite, not its faith.
TEST(CK3WorldCountyDetailsTests, RitesCanBeLinked)  // NOLINT : clang-tidy doens't like gtest
{
   std::stringstream input;
   input << "counties = {\n";
   input << "c_middlesex = { development = 9 culture = 231 rite = 0 }\n";
   input << "}";
   ck3::CountyDetails details(input);

   ck3::Religions religions;
   std::stringstream faith_input;
   faith_input << "database={ 0={ tag=\"akom_pagan\" } 12={ tag=\"christian_faith\" main_rite=0 } }";
   religions.ParseFaithDatabase(faith_input);
   std::stringstream rite_input;
   rite_input << "database={ 0={ rite_type=\"roman_rite\" faith=12 source=historical } }";
   religions.ParseRites(rite_input);
   religions.LinkReligions();

   details.LinkReligions(religions);
   // Rite 0, not faith 0.
   const auto faith = details.GetCountyDetails().at("c_middlesex")->GetFaith().GetPointer().lock();
   ASSERT_NE(nullptr, faith);
   EXPECT_EQ("roman_rite", faith->GetTag());
}

TEST(CK3WorldCountyDetailsTests, LinkingMissingFaithIsIgnored)  // NOLINT : clang-tidy doens't like gtest
{
   std::stringstream input;
   input << "counties = {\n";
   input << "c_county2 = { development = 8 culture = 3 faith = 9}\n";
   input << "c_county3 = { culture = 3 faith = 6 }\n";  // 6 is missing
   input << "}";
   ck3::CountyDetails details(input);

   std::stringstream input2;
   input2 << "faiths={\n";
   input2 << "9={tag=\"old_bon\"}\n";
   input2 << "4={tag=\"theravada\"}\n";
   input2 << "}";
   const ck3::Religions religions(input2);

   EXPECT_NO_THROW(details.LinkReligions(religions));  // NOLINT : clang-tidy doens't like gtest
}

TEST(CK3WorldCountyDetailsTests, CulturesCanBeLinked)  // NOLINT : clang-tidy doens't like gtest
{
   std::stringstream input;
   input << "counties = {\n";
   input << "c_county2 = { development = 8 culture = 13 faith = 9}\n";
   input << "c_county3 = { culture = 15 faith = 9 }\n";
   input << "}";
   ck3::CountyDetails details(input);

   std::stringstream input2;
   input2 << "cultures={\n";
   input2 << "\t13={culture_template=\"akan\"}\n";
   input2 << "\t15={culture_template=\"kru\"}\n";
   input2 << "}\n";
   const ck3::Cultures cultures(input2);

   details.LinkCultures(cultures);
   const auto& county2 = details.GetCountyDetails().find("c_county2");
   const auto& county3 = details.GetCountyDetails().find("c_county3");

   ASSERT_EQ("akan", county2->second->GetCulture().GetPointer().lock()->GetTemplate());
   ASSERT_EQ("kru", county3->second->GetCulture().GetPointer().lock()->GetTemplate());
}

TEST(CK3WorldCountyDetailsTests, LinkingMissingCultureIsIgnored)  // NOLINT : clang-tidy doens't like gtest
{
   std::stringstream input;
   input << "counties = {\n";
   input << "c_county2 = { development = 8 culture = 3 faith = 9}\n";
   input << "c_county3 = { culture = 6 faith = 9 }\n";  // 6 is missing
   input << "}";
   ck3::CountyDetails details(input);

   std::stringstream input2;
   input2 << "cultures={\n";
   input2 << "\t13={culture_template=\"akan\"}\n";
   input2 << "\t15={culture_template=\"kru\"}\n";
   input2 << "}\n";
   const ck3::Cultures cultures(input2);

   EXPECT_NO_THROW(details.LinkCultures(cultures));  // NOLINT : clang-tidy doens't like gtest
}