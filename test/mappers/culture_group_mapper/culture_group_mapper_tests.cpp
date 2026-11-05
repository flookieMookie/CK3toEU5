#include <sstream>

#include "gtest/gtest.h"
#include "src/mappers/culture_group_mapper/culture_group_mapper.hpp"

namespace mappers
{

TEST(MappersCultureGroupMapperTests, MappingsDefaultToEmpty)  // NOLINT : clang-tidy doens't like gtest
{
   std::stringstream input;
   const CultureGroupMapper mapper(input);

   ASSERT_TRUE(mapper.GetHeritageMappings().empty());
   ASSERT_TRUE(mapper.GetEU5CultureGroups("heritage_arabic").empty());
}

TEST(MappersCultureGroupMapperTests, HeritageMapsToOneGroup)  // NOLINT : clang-tidy doens't like gtest
{
   std::stringstream input;
   input << "link = { eu5 = arabic_group heritage = heritage_arabic }\n";
   const CultureGroupMapper mapper(input);

   ASSERT_EQ(1, mapper.GetEU5CultureGroups("heritage_arabic").size());
   ASSERT_EQ("arabic_group", mapper.GetEU5CultureGroups("heritage_arabic").front());
}

TEST(MappersCultureGroupMapperTests, HeritageMapsToSeveralGroups)  // NOLINT : clang-tidy doens't like gtest
{
   // This file is reversed: several eu5 groups per single ck3 heritage.
   std::stringstream input;
   input << "link = { eu5 = amazigh_group eu5 = maghrebi_group heritage = heritage_berber }\n";
   const CultureGroupMapper mapper(input);

   const auto& groups = mapper.GetEU5CultureGroups("heritage_berber");
   ASSERT_EQ(2, groups.size());
   ASSERT_EQ("amazigh_group", groups[0]);
   ASSERT_EQ("maghrebi_group", groups[1]);
}

TEST(MappersCultureGroupMapperTests, LanguageMapsToGroups)  // NOLINT : clang-tidy doens't like gtest
{
   std::stringstream input;
   input << "link = { eu5 = celtic_group language = language_gaelic }\n";
   const CultureGroupMapper mapper(input);

   ASSERT_EQ(1, mapper.GetEU5CultureGroupsForLanguage("language_gaelic").size());
   ASSERT_TRUE(mapper.GetEU5CultureGroups("language_gaelic").empty());
}

// A link naming a heritage and a language applies only where both meet: West Slavs who speak Czech are
// Bohemian, and other West Slavs are not, nor other Czech speakers.
TEST(MappersCultureGroupMapperTests, HeritageAndLanguageOnOneLinkNeedBoth)  // NOLINT : clang-tidy doens't like gtest
{
   std::stringstream input;
   input << "link = { eu5 = slavic_group heritage = heritage_west_slavic }\n";
   input << "link = { eu5 = bohemian_group heritage = heritage_west_slavic language = language_czech_slovak }\n";
   const CultureGroupMapper mapper(input);

   EXPECT_EQ("bohemian_group", mapper.GetEU5CultureGroups("heritage_west_slavic", "language_czech_slovak").front());
   ASSERT_EQ(1, mapper.GetEU5CultureGroups("heritage_west_slavic").size());
   EXPECT_EQ("slavic_group", mapper.GetEU5CultureGroups("heritage_west_slavic").front());
   EXPECT_TRUE(mapper.GetEU5CultureGroupsForLanguage("language_czech_slovak").empty());
   EXPECT_TRUE(mapper.GetEU5CultureGroups("heritage_west_slavic", "language_polish").empty());
}

TEST(MappersCultureGroupMapperTests, AHeritageCanBeNamedAsCK3)  // NOLINT : clang-tidy doens't like gtest
{
   std::stringstream input;
   input << "link = { eu5 = inuit_group ck3 = heritage_inuit }\n";
   const CultureGroupMapper mapper(input);

   ASSERT_FALSE(mapper.GetEU5CultureGroups("heritage_inuit").empty());
   EXPECT_EQ("inuit_group", mapper.GetEU5CultureGroups("heritage_inuit").front());
}

TEST(MappersCultureGroupMapperTests, NameListMapsToGroups)  // NOLINT : clang-tidy doens't like gtest
{
   std::stringstream input;
   input << "link = { eu5 = hibernian_group name_list = name_list_irish }\n";
   const CultureGroupMapper mapper(input);

   ASSERT_EQ("hibernian_group", mapper.GetEU5CultureGroupsForNameList("name_list_irish").front());
   ASSERT_TRUE(mapper.GetEU5CultureGroups("name_list_irish").empty());
}

TEST(MappersCultureGroupMapperTests, PlaceholderGroupsAreSkipped)  // NOLINT : clang-tidy doens't like gtest
{
   std::stringstream input;
   input << "link = { eu5 = ??? heritage = heritage_akan }\n";
   input << "link = { eu5 = arabic_group heritage = heritage_arabic }\n";
   const CultureGroupMapper mapper(input);

   ASSERT_TRUE(mapper.GetEU5CultureGroups("heritage_akan").empty());
   ASSERT_EQ(1, mapper.GetHeritageMappings().size());
}

}  // namespace mappers
