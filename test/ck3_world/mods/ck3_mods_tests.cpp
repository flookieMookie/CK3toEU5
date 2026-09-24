#include <filesystem>
#include <fstream>
#include <string>

#include "gtest/gtest.h"
#include "src/ck3_world/cultures/culture_localization.hpp"
#include "src/ck3_world/mods/ck3_mods.hpp"

namespace ck3
{

namespace
{
// A CK3 documents folder with one installed mod, as the CK3 launcher leaves it: a descriptor in
// mod/ pointing at the mod's own folder.
class CK3ModsTests: public testing::Test
{
  protected:
   void SetUp() override
   {
      root_ = std::filesystem::temp_directory_path() / ("ck3_mods_tests_" + std::to_string(std::rand()));
      documents_ = root_ / "documents";
      mod_folder_ = root_ / "workshop" / "12345";
      std::filesystem::create_directories(documents_ / "mod");
      std::filesystem::create_directories(mod_folder_ / "common" / "landed_titles");
      std::filesystem::create_directories(mod_folder_ / "localization" / "english" / "culture");
      Write(documents_ / "mod" / "ugc_12345.mod",
          "name=\"Test Realms\"\npath=\"" + mod_folder_.generic_string() + "\"\nsupported_version=\"1.19.*\"\n");
      Write(mod_folder_ / "common" / "landed_titles" / "00_test_titles.txt", "e_test = { }\n");
      Write(mod_folder_ / "localization" / "english" / "culture" / "test_cultures_l_english.yml",
          "\xEF\xBB\xBFl_english:\n test_culture: \"Testish\"\n");
      Write(mod_folder_ / "localization" / "english" / "test_events_l_english.yml",
          "\xEF\xBB\xBFl_english:\n test_event: \"Not a culture\"\n");
   }

   void TearDown() override
   {
      std::error_code error;
      std::filesystem::remove_all(root_, error);
   }

   static void Write(const std::filesystem::path& path, const std::string& contents)
   {
      std::ofstream file(path, std::ios::binary);
      file << contents;
   }

   std::filesystem::path root_;
   std::filesystem::path documents_;
   std::filesystem::path mod_folder_;
};
}  // namespace

TEST_F(CK3ModsTests, TheSavesModsAreFoundByTheirDescriptors)  // NOLINT : clang-tidy doens't like gtest
{
   const auto mods = ResolveMods(documents_, {"mod/ugc_12345.mod", "mod/ugc_99999.mod"});

   ASSERT_EQ(1, mods.size());
   EXPECT_EQ("Test Realms", mods[0].name);
   EXPECT_TRUE(std::filesystem::equivalent(mod_folder_, mods[0].path));
}

TEST_F(CK3ModsTests, NoModFolderMeansNoMods)  // NOLINT : clang-tidy doens't like gtest
{
   EXPECT_TRUE(ResolveMods(root_ / "nowhere", {"mod/ugc_12345.mod"}).empty());
   EXPECT_TRUE(ResolveMods(documents_, {}).empty());
}

TEST_F(CK3ModsTests, ModFilesSitOverTheGamesOwn)  // NOLINT : clang-tidy doens't like gtest
{
   const auto files = CK3Files(root_ / "ck3", ResolveMods(documents_, {"mod/ugc_12345.mod"}));

   const auto titles = files.GetAllFilesInFolder("common/landed_titles");

   ASSERT_EQ(1, titles.size());
   EXPECT_EQ("00_test_titles.txt", titles[0].filename().string());
}

TEST_F(CK3ModsTests, AModWithItsOwnMapIsNoticed)  // NOLINT : clang-tidy doens't like gtest
{
   const auto mods = ResolveMods(documents_, {"mod/ugc_12345.mod"});
   ASSERT_EQ(1, mods.size());
   EXPECT_FALSE(ChangesMap(mods[0]));

   std::filesystem::create_directories(mod_folder_ / "map_data");
   Write(mod_folder_ / "map_data" / "definition.csv", "0;0;0;0;x;x;\n");

   EXPECT_TRUE(ChangesMap(mods[0]));
}

TEST_F(CK3ModsTests, ModsNameTheirOwnCultures)  // NOLINT : clang-tidy doens't like gtest
{
   const auto names = LoadCultureLocalization(root_ / "ck3", ResolveMods(documents_, {"mod/ugc_12345.mod"}));

   ASSERT_TRUE(names.GetLocalizationBlock("test_culture").has_value());
   EXPECT_EQ("Testish", names.GetLocalizationBlock("test_culture")->GetLocalization("english"));
   // Only the files about cultures are read.
   EXPECT_FALSE(names.GetLocalizationBlock("test_event").has_value());
}

}  // namespace ck3
