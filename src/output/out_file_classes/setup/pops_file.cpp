#include "pops_file.hpp"

#include <external/commonItems/Log.h>

#include <format>
#include <sstream>
#include <string>

namespace out
{

PopsFile::PopsFile(const std::string& name,
    FileWriter& file_writer,
    const eu5::EU5World& eu5_world,
    const eu5::LocationData& location_data):
    OutputFile(name, file_writer),
    eu5_world_(eu5_world),
    location_data_(location_data)
{
}

void PopsFile::Create(const std::filesystem::path& folder_path)
{
   Log(LogLevel::Info) << "\tCreating " << GetName();

   const auto& location_religions = eu5_world_.GetLocationReligions();

   std::ostringstream output;
   output << "# Populations with religion converted from the CK3 save. Types, sizes and cultures\n";
   output << "# are EU5's own, since there is no CK3 to EU5 culture mapping to convert them with.\n\n";
   output << "locations = {\n";

   int converted_locations = 0;
   int converted_pops = 0;
   for (const auto& [location, pops]: location_data_.GetPops())
   {
      output << "\n\t" << location << " = {\n";

      const auto converted_religion = location_religions.find(location);
      const bool convert = converted_religion != location_religions.end();
      if (convert)
      {
         ++converted_locations;
      }
      // Only the majority faith is converted. EU5's own minorities - the Jewish burghers of Paris,
      // for instance - have no counterpart in CK3's single faith per county, and overwriting them
      // would quietly erase them from the map.
      const auto vanilla_majority = location_data_.GetDominantReligion(location);

      for (const auto& pop: pops)
      {
         const bool convert_this_pop = convert && pop.religion == vanilla_majority;
         const auto& religion = convert_this_pop ? converted_religion->second : pop.religion;
         if (convert_this_pop && religion != pop.religion)
         {
            ++converted_pops;
         }
         output << std::format("\t\tdefine_pop = {{ type = {} size = {:.3f} culture = {} religion = {} }}\n",
             pop.type,
             pop.size,
             pop.culture,
             religion);
      }
      output << "\t}\n";
   }

   output << "}\n";

   Log(LogLevel::Info) << "\t<> Wrote pops for " << location_data_.GetPops().size() << " locations, converting "
                       << converted_pops << " pops across " << converted_locations << " locations.";
   UseFileWriter().CreateEmptyAndWrite(folder_path / GetName(), output.str());
}

}  // namespace out
