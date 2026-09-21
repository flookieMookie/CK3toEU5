#include "countries_file.hpp"

#include <external/commonItems/Log.h>

#include <sstream>
#include <string>

#include "src/ck3_world/realms/realm.hpp"
#include "src/eu5_world/eu5_country.hpp"

namespace
{
// Vanilla's own 10_countries.txt wraps the blocks in an outer countries = { countries = { ... } }
// and keeps the location lists to readable line lengths.
constexpr int kLocationsPerLine = 8;

// Matches vanilla's first age. Which age a converted save should start in is still undecided.
const std::string kCurrentAge = "age_1_traditions";
}  // namespace

namespace out
{

CountriesFile::CountriesFile(const std::string& name, FileWriter& file_writer, const eu5::EU5World& eu5_world):
    OutputFile(name, file_writer),
    eu5_world_(eu5_world)
{
}

void CountriesFile::Create(const std::filesystem::path& folder_path)
{
   Log(LogLevel::Info) << "\tCreating " << GetName();

   std::ostringstream output;
   output << "current_age = " << kCurrentAge << "\n\n";
   output << "countries = {\n";
   output << "\tcountries = {\n";

   for (const auto& country: eu5_world_.GetCountries())
   {
      if (country->GetLocations().empty())
      {
         continue;
      }
      output << "\n\t\t" << country->GetTag() << " = { # " << country->GetSourceRealm()->GetRealmName() << "\n";
      output << "\t\t\town_control_core = {\n";

      int on_this_line = 0;
      for (const auto& location: country->GetLocations())
      {
         if (on_this_line == 0)
         {
            output << "\t\t\t\t";
         }
         output << location << " ";
         if (++on_this_line == kLocationsPerLine)
         {
            output << "\n";
            on_this_line = 0;
         }
      }
      if (on_this_line != 0)
      {
         output << "\n";
      }

      output << "\t\t\t}\n";
      output << "\t\t}\n";
   }

   output << "\t}\n";
   output << "}\n";

   UseFileWriter().CreateEmptyAndWrite(folder_path / GetName(), output.str());
}

}  // namespace out
