#include "wars_file.hpp"

#include <external/commonItems/Log.h>

#include <fstream>
#include <iterator>
#include <set>
#include <sstream>
#include <string>
#include <utility>

#include "vanilla_start_file.hpp"

namespace
{
// EU5 starts on 1337.4.1; its own wars last acted on 1337.3.1, and none may start after that.
const date kGameStartDate = date("1337.4.1");
const date kLastAction = date("1337.3.1");

void WriteSide(std::ostringstream& output, const std::string& side, const std::vector<eu5::WarParticipant>& participants)
{
   for (const auto& participant: participants)
   {
      output << "\t\t" << side << " = {\n";
      output << "\t\t\tcountry = " << participant.tag << "\n";
      output << "\t\t\trequest = {\n";
      if (!participant.caller.empty())
      {
         output << "\t\t\t\tcaller = " << participant.caller << "\n";
      }
      output << "\t\t\t\treason = " << participant.reason << "\n";
      if (participant.reason == "Scripted")
      {
         output << "\t\t\t\twhich = alliance\n";
      }
      output << "\t\t\t}\n";
      output << "\t\t}\n";
   }
}
}  // namespace

std::string out::WriteWar(const eu5::ConvertedWar& war, const date& conversion_date)
{
   auto start_date = war.start_date;
   start_date.ChangeByYears(kGameStartDate.getYear() - conversion_date.getYear());
   if (kLastAction < start_date)
   {
      start_date = kLastAction;
   }

   std::ostringstream output;
   output << "\twar = {";
   if (!war.name.empty())
   {
      output << " # " << war.name;
   }
   output << "\n";
   output << "\t\twar_name = {\n";
   output << "\t\t\tname = \"" << (war.name_key.empty() ? "NORMAL_WAR_NAME" : war.name_key) << "\"\n";
   output << "\t\t\tordinal = 1\n";
   output << "\t\t\tfirst = {\n\t\t\t\tname = \"" << war.attackers.front().tag << "\"\n\t\t\t}\n";
   output << "\t\t\tsecond = {\n\t\t\t\tname = \"" << war.defenders.front().tag << "\"\n\t\t\t}\n";
   output << "\t\t}\n\n";
   output << "\t\ttake_province = {\n";
   output << "\t\t\ttype = conquer_province\n";
   output << "\t\t\tcasus_belli = cb_conquer_province\n";
   output << "\t\t\tlocation = " << war.target_location << "\n";
   output << "\t\t}\n\n";
   output << "\t\tstart_date = " << start_date << "\n";
   output << "\t\taction = " << kLastAction << "\n";
   WriteSide(output, "attacker", war.attackers);
   WriteSide(output, "defender", war.defenders);
   output << "\t}\n";
   return output.str();
}

std::string out::WriteTruce(const eu5::ConvertedTruce& truce)
{
   std::ostringstream output;
   output << "\ttruce = {\n";
   output << "\t\tattacker = " << truce.first_tag << "\n";
   output << "\t\tdefender = " << truce.second_tag << "\n";
   output << "\t\tstart_date = " << kLastAction << "\n";
   output << "\t\tmonths = " << truce.months << "\n";
   output << "\t}\n";
   return output.str();
}

std::string out::WriteStandingArmies(const std::map<std::string, int>& regiments,
    const std::map<std::string, std::string>& capitals)
{
   std::ostringstream output;
   for (const auto& [tag, count]: regiments)
   {
      const auto capital = capitals.find(tag);
      if (capital == capitals.end() || count <= 0)
      {
         continue;
      }
      output << "\n\tarmy = {\n";
      output << "\t\tcountry = " << tag << "\n";
      output << "\t\tlocation = " << capital->second << "\n";
      output << "\t\tsub_units = {\n";
      for (int regiment = 0; regiment < count; ++regiment)
      {
         output << "\t\t\ta_footmen = { strength = 1 }\n";
      }
      output << "\t\t}\n";
      output << "\t}\n";
   }
   return output.str();
}

std::string out::WriteLevies(const std::vector<eu5::ConvertedWar>& wars,
    const std::map<std::string, std::string>& capitals,
    const eu5::MapAreas& map_areas)
{
   std::set<std::string> raised;
   std::ostringstream output;
   for (const auto& war: wars)
   {
      for (const auto* side: {&war.attackers, &war.defenders})
      {
         for (const auto& participant: *side)
         {
            const auto capital = capitals.find(participant.tag);
            if (capital == capitals.end() || !raised.insert(participant.tag).second)
            {
               continue;
            }
            const auto area = map_areas.AreaOf(capital->second);
            if (!area.has_value())
            {
               continue;
            }
            output << "\n\tlevy = {\n";
            output << "\t\tcountry = " << participant.tag << "\n";
            output << "\t\tarea = " << *area << "\n";
            output << "\t\tlevy = 1\n";
            output << "\t\tlocation = " << capital->second << "\n";
            output << "\t}\n";
         }
      }
   }
   return output.str();
}

namespace out
{

WarsFile::WarsFile(const std::string& name,
    FileWriter& file_writer,
    const eu5::EU5World& eu5_world,
    const eu5::VanillaCountries& vanilla_countries,
    std::filesystem::path eu5_directory):
    OutputFile(name, file_writer),
    eu5_world_(eu5_world),
    vanilla_countries_(vanilla_countries),
    eu5_directory_(std::move(eu5_directory))
{
}

void WarsFile::Create(const std::filesystem::path& folder_path)
{
   Log(LogLevel::Info) << "\tCreating " << GetName();

   std::string contents = "war_manager = {\n}\n";
   if (std::ifstream vanilla(eu5_directory_ / "game" / "main_menu" / "setup" / "start" / GetName()); vanilla.is_open())
   {
      contents.assign(std::istreambuf_iterator<char>(vanilla), std::istreambuf_iterator<char>());
      if (contents.starts_with("\xEF\xBB\xBF"))
      {
         contents.erase(0, 3);
      }
   }

   // EU5's own wars stay only among the vanilla countries kept as they are.
   std::set<std::string> kept_tags;
   for (const auto* country: vanilla_countries_.GetUntouched(eu5_world_.GetConvertedLocations()))
   {
      kept_tags.insert(country->tag);
   }
   auto wars = KeepEntriesAbout(contents, 1, kept_tags);

   // The converted ones go inside war_manager, before its closing brace.
   std::string converted;
   for (const auto& war: eu5_world_.GetWars())
   {
      converted += "\n" + WriteWar(war, eu5_world_.GetConversionDate());
   }
   for (const auto& truce: eu5_world_.GetTruces())
   {
      converted += "\n" + WriteTruce(truce);
   }
   const auto closing = wars.rfind('}');
   wars.insert(closing == std::string::npos ? wars.size() : closing, converted);

   Log(LogLevel::Info) << "\t<> Wrote " << eu5_world_.GetWars().size() << " wars and " << eu5_world_.GetTruces().size()
                       << " truces from the CK3 save.";
   UseFileWriter().CreateEmptyAndWrite(folder_path / GetName(), "\xEF\xBB\xBF" + wars);
}

}  // namespace out
