#include "coats_of_arms_file.hpp"

#include <external/commonItems/Log.h>

#include <array>
#include <regex>
#include <set>
#include <sstream>
#include <string>
#include <system_error>
#include <utility>

namespace
{
// Where Jomini keeps coat of arms art, in both games.
const std::array<std::string, 3> kTextureFolders = {"patterns", "colored_emblems", "textured_emblems"};

// CK3 sometimes orders overlapping emblems with depth, which none of EU5's own coats of arms use.
// It only fine tunes layering, so it is dropped rather than risk EU5 rejecting a key it doesn't
// expect.
const std::regex kDepth(R"(\bdepth\s*=\s*[-0-9.]+)");
}  // namespace

namespace out
{

CoatsOfArmsFile::CoatsOfArmsFile(const std::string& name, FileWriter& file_writer, const eu5::EU5World& eu5_world):
    OutputFile(name, file_writer),
    eu5_world_(eu5_world)
{
}

void CoatsOfArmsFile::Create(const std::filesystem::path& folder_path)
{
   Log(LogLevel::Info) << "\tCreating " << GetName();

   std::ostringstream output;
   output << "# Coats of arms converted from the CK3 save, for the countries whose tag EU5 does not define.\n";
   for (const auto& [tag, coat_of_arms]: eu5_world_.GetFlags())
   {
      output << "\n" << tag << " = " << std::regex_replace(coat_of_arms.definition, kDepth, "") << "\n";
   }

   Log(LogLevel::Info) << "\t<> Wrote " << eu5_world_.GetFlags().size() << " coats of arms.";
   UseFileWriter().CreateEmptyAndWrite(folder_path / GetName(), output.str());
}

CoatOfArmsTextures::CoatOfArmsTextures(const std::string& name,
    FileWriter& file_writer,
    const eu5::EU5World& eu5_world,
    commonItems::ModFilesystem ck3_files,
    std::filesystem::path eu5_directory):
    OutputFile(name, file_writer),
    eu5_world_(eu5_world),
    ck3_files_(std::move(ck3_files)),
    eu5_directory_(std::move(eu5_directory))
{
}

void CoatOfArmsTextures::Create(const std::filesystem::path& folder_path)
{
   Log(LogLevel::Info) << "\tCopying coat of arms art EU5 lacks from CK3";

   std::set<std::string> textures;
   for (const auto& [tag, coat_of_arms]: eu5_world_.GetFlags())
   {
      textures.insert(coat_of_arms.textures.begin(), coat_of_arms.textures.end());
   }

   const auto eu5_art = eu5_directory_ / "game" / "main_menu" / "gfx" / "coat_of_arms";
   int copied = 0;
   int missing = 0;
   std::error_code error;
   for (const auto& texture: textures)
   {
      bool found = false;
      for (const auto& subfolder: kTextureFolders)
      {
         if (std::filesystem::is_regular_file(eu5_art / subfolder / texture, error))
         {
            found = true;  // EU5 has it already
            break;
         }
         // CK3's own art, or a mod's where the save used one that adds or redraws it.
         if (const auto source =
                 ck3_files_.GetActualFileLocation(std::filesystem::path("gfx") / "coat_of_arms" / subfolder / texture))
         {
            std::filesystem::create_directories(folder_path / subfolder, error);
            std::filesystem::copy_file(*source,
                folder_path / subfolder / texture,
                std::filesystem::copy_options::overwrite_existing,
                error);
            if (!error)
            {
               ++copied;
               found = true;
               break;
            }
         }
      }
      if (!found)
      {
         ++missing;
      }
   }
   Log(LogLevel::Info) << "\t<> Copied " << copied << " patterns and emblems from CK3; " << missing
                       << " could not be found in either game.";
}

}  // namespace out
