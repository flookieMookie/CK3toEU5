#ifndef EU5_SETUP_FOLDER_H
#define EU5_SETUP_FOLDER_H

#include <filesystem>
#include <string>
#include <system_error>

namespace eu5
{

// The folder below main_menu/setup that EU5's 1337 start is read from. It was start until EU5 gave
// each start date a folder of its own, setup/1337 for 1337. The mod mirrors whichever this install
// has, so its files take the place of the game's own.
[[nodiscard]] inline std::string StartSetupFolderName(const std::filesystem::path& eu5_directory)
{
   std::error_code error;
   if (std::filesystem::is_directory(eu5_directory / "game" / "main_menu" / "setup" / "1337", error))
   {
      return "1337";
   }
   return "start";
}

// That folder in the EU5 install: game/main_menu/setup/1337, or setup/start in an older EU5.
[[nodiscard]] inline std::filesystem::path StartSetupFolder(const std::filesystem::path& eu5_directory)
{
   return eu5_directory / "game" / "main_menu" / "setup" / StartSetupFolderName(eu5_directory);
}

}  // namespace eu5

#endif  // EU5_SETUP_FOLDER_H
