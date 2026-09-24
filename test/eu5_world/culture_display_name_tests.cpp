#include <external/commonItems/Localization/LocalizationDatabase.h>

#include <sstream>

#include "gtest/gtest.h"
#include "src/eu5_world/eu5_culture_resolver.hpp"

namespace eu5
{

namespace
{
commonItems::LocalizationDatabase MakeCK3Names()
{
   commonItems::LocalizationDatabase names("english", {"french"});
   std::stringstream english;
   english << "l_english:\n norse:0 \"Norse\"\n built:0 \"$norse$ Folk\"\n";
   std::stringstream french;
   french << "l_french:\n norse:0 \"Nordique\"\n";
   (void)names.ScrapeStream(english);
   (void)names.ScrapeStream(french);
   return names;
}
}  // namespace

TEST(EU5WorldCultureDisplayNameTests, CK3NameIsUsedInEachLanguage)  // NOLINT : clang-tidy doens't like gtest
{
   const auto names = MakeCK3Names();
   const CultureDefinition norse{"old_norse_language", {"scandinavian_group"}, "norse", "norse"};

   EXPECT_EQ("Norse", CultureDisplayName("norse", norse, names, "english"));
   EXPECT_EQ("Nordique", CultureDisplayName("norse", norse, names, "french"));
   // A language CK3 doesn't ship falls back to English.
   EXPECT_EQ("Norse", CultureDisplayName("norse", norse, names, "turkish"));
}

TEST(EU5WorldCultureDisplayNameTests, CampaignCulturesKeepTheirOwnName)  // NOLINT : clang-tidy doens't like gtest
{
   const auto names = MakeCK3Names();
   const CultureDefinition hybrid{"old_norse_language", {"scandinavian_group"}, "", "Anglo-Norse"};

   EXPECT_EQ("Anglo-Norse", CultureDisplayName("anglo_norse", hybrid, names, "english"));
}

TEST(EU5WorldCultureDisplayNameTests, NamesBuiltFromOtherKeysAreNotShownRaw)  // NOLINT : clang-tidy doens't like gtest
{
   const auto names = MakeCK3Names();
   const CultureDefinition built{"x_language", {"x_group"}, "built", "built"};

   EXPECT_EQ("Built", CultureDisplayName("built", built, names, "english"));
}

TEST(EU5WorldCultureDisplayNameTests, UnknownCulturesAreNamedAfterTheirKey)  // NOLINT : clang-tidy doens't like gtest
{
   const auto names = MakeCK3Names();
   const CultureDefinition unknown{"x_language", {"x_group"}, "old_saxon", "old_saxon"};

   EXPECT_EQ("Old Saxon", CultureDisplayName("old_saxon", unknown, names, "english"));
}

}  // namespace eu5
