#include <filesystem>
#include <fstream>
#include <sstream>

#include "gtest/gtest.h"
#include "src/launcher/launcher_core.hpp"

namespace launcher
{

namespace
{
// A throwaway folder for tests that need real files, removed again when the test ends.
class ScratchFolder
{
  public:
   explicit ScratchFolder(const std::string& name):
       path_(std::filesystem::temp_directory_path() / ("ck3toeu5_launcher_" + name))
   {
      std::filesystem::remove_all(path_);
      std::filesystem::create_directories(path_);
   }
   ~ScratchFolder() { std::filesystem::remove_all(path_); }
   ScratchFolder(const ScratchFolder&) = delete;
   ScratchFolder& operator=(const ScratchFolder&) = delete;
   ScratchFolder(ScratchFolder&&) = delete;
   ScratchFolder& operator=(ScratchFolder&&) = delete;

   [[nodiscard]] const auto& GetPath() const { return path_; }

  private:
   std::filesystem::path path_;
};
}  // namespace

TEST(LauncherCoreTests, ConfigurationHasEveryKeyTheConverterReads)  // NOLINT : clang-tidy doens't like gtest
{
   Settings settings;
   settings.ck3_install = "C:\\Games\\Crusader Kings III";
   settings.ck3_documents = "C:\\Docs\\Crusader Kings III";
   settings.eu5_install = "C:\\Games\\Europa Universalis V";
   settings.eu5_mods = "C:\\Docs\\Europa Universalis V\\mod";
   settings.save_game = "C:\\Docs\\Crusader Kings III\\save games\\my game.ck3";
   settings.mod_name = "My Mod";

   const auto configuration = MakeConverterConfiguration(settings);

   EXPECT_NE(std::string::npos, configuration.find("CK3directory = \"C:/Games/Crusader Kings III\""));
   EXPECT_NE(std::string::npos, configuration.find("CK3DocDirectory = \"C:/Docs/Crusader Kings III\""));
   EXPECT_NE(std::string::npos, configuration.find("EU5directory = \"C:/Games/Europa Universalis V\""));
   EXPECT_NE(std::string::npos, configuration.find("targetGameModPath = \"C:/Docs/Europa Universalis V/mod\""));
   EXPECT_NE(std::string::npos,
       configuration.find("SaveGame = \"C:/Docs/Crusader Kings III/save games/my game.ck3\""));
   EXPECT_NE(std::string::npos, configuration.find("output_name = \"My Mod\""));
   EXPECT_NE(std::string::npos, configuration.find("debug = \"no\""));
}

TEST(LauncherCoreTests, QuotesInTheModNameCannotBreakTheConfiguration)  // NOLINT : clang-tidy doens't like gtest
{
   Settings settings;
   settings.mod_name = R"(The "Great" Mod)";

   EXPECT_NE(std::string::npos, MakeConverterConfiguration(settings).find("output_name = \"The Great Mod\""));
}

TEST(LauncherCoreTests, ConfigurationKeepsNonAsciiPathsAsUtf8)  // NOLINT : clang-tidy doens't like gtest
{
   Settings settings;
   settings.save_game = FromUtf8("C:/Users/Jos\xC3\xA9/save.ck3");

   EXPECT_NE(std::string::npos, MakeConverterConfiguration(settings).find("C:/Users/Jos\xC3\xA9/save.ck3"));
}

TEST(LauncherCoreTests, LogLinesSplitIntoLevelAndMessage)  // NOLINT : clang-tidy doens't like gtest
{
   const auto line = ParseLogLine("2026-09-23 18:40:14     [INFO] \tUsing output name My_Mod\r");

   EXPECT_EQ(LogLevel::kInfo, line.level);
   EXPECT_EQ("Using output name My_Mod", line.message);
   EXPECT_EQ(LogLevel::kWarning, ParseLogLine("2026-09-23 18:40:14  [WARNING] careful").level);
   EXPECT_EQ(LogLevel::kError, ParseLogLine("2026-09-23 18:40:14    [ERROR] broken").level);
   EXPECT_EQ(LogLevel::kNotice, ParseLogLine("2026-09-23 18:40:14   [NOTICE] * Conversion complete *").level);
}

TEST(LauncherCoreTests, LinesWithoutALevelAreKeptAsUnknown)  // NOLINT : clang-tidy doens't like gtest
{
   const auto line = ParseLogLine("  something crashed  ");

   EXPECT_EQ(LogLevel::kUnknown, line.level);
   EXPECT_EQ("something crashed", line.message);
   EXPECT_EQ(LogLevel::kUnknown, ParseLogLine("[odd] bracketed text").level);
}

TEST(LauncherCoreTests, ProgressIsReadFromProgressLinesOnly)  // NOLINT : clang-tidy doens't like gtest
{
   EXPECT_EQ(85, ParseProgress(ParseLogLine("2026-09-23 18:40:14 [PROGRESS] 85%")));
   EXPECT_EQ(100, ParseProgress(ParseLogLine("2026-09-23 18:40:14 [PROGRESS] 100%")));
   EXPECT_FALSE(ParseProgress(ParseLogLine("2026-09-23 18:40:14     [INFO] 85%")).has_value());
   EXPECT_FALSE(ParseProgress(ParseLogLine("2026-09-23 18:40:14 [PROGRESS] soon")).has_value());
}

TEST(LauncherCoreTests, OutputNameIsReadFromTheConverter)  // NOLINT : clang-tidy doens't like gtest
{
   EXPECT_EQ("CLAUDE_SAVE_FILE_9_21_26",
       ParseOutputName(ParseLogLine("2026-09-23 18:40:14     [INFO] \tUsing output name CLAUDE_SAVE_FILE_9_21_26")));
   EXPECT_FALSE(ParseOutputName(ParseLogLine("2026-09-23 18:40:14     [INFO] Using something else")).has_value());
   EXPECT_FALSE(ParseOutputName(ParseLogLine("2026-09-23 18:40:14  [WARNING] Using output name X")).has_value());
}

TEST(LauncherCoreTests, SteamLibraryPathsAreUnescaped)  // NOLINT : clang-tidy doens't like gtest
{
   std::stringstream vdf;
   vdf << "\"libraryfolders\"\n{\n";
   vdf << "\t\"0\"\n\t{\n\t\t\"path\"\t\t\"C:\\\\Program Files (x86)\\\\Steam\"\n\t}\n";
   vdf << "\t\"1\"\n\t{\n\t\t\"path\"\t\t\"D:\\\\SteamLibrary\"\n\t\t\"label\"\t\t\"\"\n\t}\n";
   vdf << "}\n";

   const auto libraries = ParseSteamLibraryFolders(vdf);

   ASSERT_EQ(2, libraries.size());
   EXPECT_EQ(std::filesystem::path("C:\\Program Files (x86)\\Steam"), libraries[0]);
   EXPECT_EQ(std::filesystem::path("D:\\SteamLibrary"), libraries[1]);
}

TEST(LauncherCoreTests, AGameIsFoundOnlyWhereItHasAGameFolder)  // NOLINT : clang-tidy doens't like gtest
{
   const ScratchFolder scratch("find_game");
   const auto empty_library = scratch.GetPath() / "empty";
   const auto real_library = scratch.GetPath() / "real";
   std::filesystem::create_directories(empty_library / "steamapps" / "common" / "Europa Universalis V");
   std::filesystem::create_directories(real_library / "steamapps" / "common" / "Europa Universalis V" / "game");

   const auto found = FindSteamGame({empty_library, real_library}, "Europa Universalis V");

   ASSERT_TRUE(found.has_value());
   EXPECT_EQ(real_library / "steamapps" / "common" / "Europa Universalis V", *found);
   EXPECT_FALSE(FindSteamGame({empty_library}, "Europa Universalis V").has_value());
}

TEST(LauncherCoreTests, EveryMissingPieceIsReported)  // NOLINT : clang-tidy doens't like gtest
{
   EXPECT_EQ(5, ValidateSettings(Settings{}).size());
}

TEST(LauncherCoreTests, CompleteSettingsAreAccepted)  // NOLINT : clang-tidy doens't like gtest
{
   const ScratchFolder scratch("complete");
   Settings settings;
   settings.ck3_install = scratch.GetPath() / "ck3";
   settings.ck3_documents = scratch.GetPath() / "ck3_documents";
   settings.eu5_install = scratch.GetPath() / "eu5";
   // EU5 only creates its mod folder once a mod is installed, so it need not exist yet.
   settings.eu5_mods = scratch.GetPath() / "eu5_documents" / "mod";
   settings.save_game = scratch.GetPath() / "ck3_documents" / "save.ck3";
   std::filesystem::create_directories(settings.ck3_install / "game");
   std::filesystem::create_directories(settings.eu5_install / "game");
   std::filesystem::create_directories(settings.ck3_documents);
   std::ofstream(settings.save_game) << "save";

   EXPECT_TRUE(ValidateSettings(settings).empty());
}

TEST(LauncherCoreTests, ReleaseTagsAreComparedByVersion)  // NOLINT : clang-tidy doens't like gtest
{
   EXPECT_EQ((std::vector<int>{2, 1}), VersionNumbers("preview-2.1"));
   EXPECT_EQ((std::vector<int>{3}), VersionNumbers("v3"));
   EXPECT_TRUE(VersionNumbers("nightly").empty());

   EXPECT_TRUE(IsNewerRelease("preview-2.1", "preview-2"));
   EXPECT_TRUE(IsNewerRelease("preview-2.10", "preview-2.9"));
   EXPECT_TRUE(IsNewerRelease("v3.0", "preview-2.1"));
   EXPECT_FALSE(IsNewerRelease("preview-2.0", "preview-2"));
   EXPECT_FALSE(IsNewerRelease("preview-2", "preview-2.1"));
   EXPECT_FALSE(IsNewerRelease("nightly", "preview-2.1"));
}

TEST(LauncherCoreTests, TheNewestReleaseIsFoundInGitHubsList)  // NOLINT : clang-tidy doens't like gtest
{
   const std::string releases = R"([
      {"url": "https://api.github.com/x", "tag_name": "preview-2.1", "name": "unofficial preview 2.1", "prerelease": true},
      {"tag_name":"preview-3","prerelease":false},
      {"tag_name": "preview-2", "prerelease": true},
      {"tag_name": "nightly"}
   ])";

   EXPECT_EQ("preview-3", NewestReleaseTag(releases));
   EXPECT_FALSE(NewestReleaseTag("[]").has_value());
   EXPECT_FALSE(NewestReleaseTag("").has_value());
}

TEST(LauncherCoreTests, ReleaseNamesReadLikeWords)  // NOLINT : clang-tidy doens't like gtest
{
   EXPECT_EQ("preview 2.1", ReleaseDisplayName("preview-2.1"));
   EXPECT_EQ("version 2.2", ReleaseDisplayName("v2.2"));
   EXPECT_TRUE(IsNewerRelease("v2.2", "v2.1"));
   EXPECT_FALSE(IsNewerRelease("preview-2.1", "v2.1"));
}

}  // namespace launcher
