#include "diplomacy_file.hpp"

#include <external/commonItems/Log.h>

#include <set>
#include <sstream>
#include <string>

#include "src/eu5_world/eu5_country.hpp"

namespace out
{

DiplomacyFile::DiplomacyFile(const std::string& name, FileWriter& file_writer, const eu5::EU5World& eu5_world):
    OutputFile(name, file_writer),
    eu5_world_(eu5_world)
{
}

void DiplomacyFile::Create(const std::filesystem::path& folder_path)
{
   Log(LogLevel::Info) << "\tCreating " << GetName();

   // A dependency naming a country that isn't in 10_countries would point at one EU5 does not have.
   std::set<std::string> written_tags;
   for (const auto& country: eu5_world_.GetCountries())
   {
      if (country->IsWritten())
      {
         written_tags.insert(country->GetTag());
      }
   }

   std::ostringstream output;
   output << "# Vassals, tributaries and alliances converted from the CK3 save.\n\n";
   output << "diplomacy_manager = {\n";

   int written = 0;
   for (const auto& dependency: eu5_world_.GetDependencies())
   {
      if (!written_tags.contains(dependency.liege_tag) || !written_tags.contains(dependency.vassal_tag))
      {
         continue;
      }
      output << "\tdependency = { first = " << dependency.liege_tag << " second = " << dependency.vassal_tag
             << " subject_type = " << dependency.subject_type << " }\n";
      ++written;
   }

   int alliances = 0;
   for (const auto& [first, second]: eu5_world_.GetAlliances())
   {
      if (!written_tags.contains(first) || !written_tags.contains(second))
      {
         continue;
      }
      output << "\tscripted_mutual = { first = " << first << " second = " << second << " type = alliance }\n";
      ++alliances;
   }

   output << "}\n";

   Log(LogLevel::Info) << "\t<> Wrote " << written << " subject relationships and " << alliances << " alliances.";
   UseFileWriter().CreateEmptyAndWrite(folder_path / GetName(), output.str());
}

}  // namespace out
