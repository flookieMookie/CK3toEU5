#include "diplomacy_file.hpp"

#include <external/commonItems/Log.h>

#include <set>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "src/eu5_world/eu5_country.hpp"

namespace out
{

std::string WriteSubjectsAndAlliances(const eu5::EU5World& eu5_world)
{
   // A relationship naming a country that isn't in 10_countries would point at one EU5 does not have.
   std::set<std::string> written_tags;
   for (const auto& country: eu5_world.GetCountries())
   {
      if (country->IsWritten())
      {
         written_tags.insert(country->GetTag());
      }
   }

   std::ostringstream output;
   int subjects = 0;
   for (const auto& dependency: eu5_world.GetDependencies())
   {
      if (written_tags.contains(dependency.liege_tag) && written_tags.contains(dependency.vassal_tag))
      {
         output << "\tdependency = { first = " << dependency.liege_tag << " second = " << dependency.vassal_tag
                << " subject_type = " << dependency.subject_type << " }\n";
         ++subjects;
      }
   }
   int alliances = 0;
   for (const auto& [first, second]: eu5_world.GetAlliances())
   {
      if (written_tags.contains(first) && written_tags.contains(second))
      {
         output << "\tscripted_mutual = { first = " << first << " second = " << second << " type = alliance }\n";
         ++alliances;
      }
   }
   Log(LogLevel::Info) << "\t<> Wrote " << subjects << " subject relationships and " << alliances << " alliances.";
   return output.str();
}

std::string WriteRivals(const std::vector<std::pair<std::string, std::string>>& rivals)
{
   std::ostringstream output;
   for (const auto& [first, second]: rivals)
   {
      output << "\trival = { first = " << first << " second = " << second << " }\n";
   }
   return output.str();
}

std::string WriteGoodRelations(const std::vector<std::pair<std::string, std::string>>& relations)
{
   std::ostringstream output;
   for (const auto& [first, second]: relations)
   {
      output << "\topinion = { first = " << first << " second = " << second << " type = opinion_good_relations }\n";
   }
   return output.str();
}

}  // namespace out
