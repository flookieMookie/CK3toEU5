#include "dynasties_file.hpp"

#include <external/commonItems/Log.h>

#include <fstream>
#include <iterator>
#include <sstream>
#include <string>
#include <utility>

namespace
{
const std::filesystem::path kVanillaFile =
    std::filesystem::path("game") / "main_menu" / "setup" / "start" / "04_dynasties.txt";
}  // namespace

namespace out
{

DynastiesFile::DynastiesFile(const std::string& name,
    FileWriter& file_writer,
    const eu5::EU5World& eu5_world,
    std::filesystem::path eu5_directory):
    OutputFile(name, file_writer),
    eu5_world_(eu5_world),
    eu5_directory_(std::move(eu5_directory))
{
}

void DynastiesFile::Create(const std::filesystem::path& folder_path)
{
   Log(LogLevel::Info) << "\tCreating " << GetName();

   std::ifstream vanilla(eu5_directory_ / kVanillaFile);
   if (!vanilla.is_open())
   {
      // Without EU5's own the file can't be written safely, and converted characters simply go
      // without a dynasty, which EU5 accepts.
      Log(LogLevel::Warning) << "\t<> EU5 dynasties not found - skipping, vanilla will apply.";
      return;
   }
   // The whole file is one dynasty_manager block; ours go in before its closing brace.
   const std::string contents((std::istreambuf_iterator<char>(vanilla)), std::istreambuf_iterator<char>());
   const auto last_brace = contents.find_last_of('}');
   if (last_brace == std::string::npos)
   {
      Log(LogLevel::Warning) << "\t<> EU5 dynasties look malformed - skipping.";
      return;
   }

   std::ostringstream output;
   output << contents.substr(0, last_brace);
   output << "\n\t# Houses converted from the CK3 save.\n";
   for (const auto& [house_id, dynasty]: eu5_world_.GetDynasties())
   {
      output << "\t" << dynasty.id << " = {\n";
      output << "\t\tname = { name = " << dynasty.id << " }\n";
      output << "\t\thome = " << dynasty.home << "\n";
      output << "\t}\n";
   }
   output << "}\n";

   Log(LogLevel::Info) << "\t<> Wrote " << eu5_world_.GetDynasties().size() << " converted dynasties.";
   UseFileWriter().CreateEmptyAndWrite(folder_path / GetName(), output.str());
}

}  // namespace out
