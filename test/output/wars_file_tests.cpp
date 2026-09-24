#include <sstream>
#include <string>

#include "gtest/gtest.h"
#include "src/output/out_file_classes/setup/wars_file.hpp"

namespace out
{

namespace
{
eu5::ConvertedWar MakeWar()
{
   eu5::ConvertedWar war;
   war.name_key = "ck3_war_0";
   war.name = "Aghlabid Conquest of Sicily";
   war.start_date = date("866.5.2");
   war.target_location = "palermo";
   war.attackers = {{"TUN", "Instigator", ""}, {"MLT", "Subject", "TUN"}, {"EGY", "Scripted", "TUN"}};
   war.defenders = {{"BYZ", "Target", ""}};
   return war;
}
}  // namespace

TEST(OutputWarsFileTests, AWarIsWrittenAsAConquestOfItsTarget)  // NOLINT : clang-tidy doens't like gtest
{
   const auto written = WriteWar(MakeWar(), date("867.1.1"));

   EXPECT_EQ(
       "\twar = { # Aghlabid Conquest of Sicily\n"
       "\t\twar_name = {\n"
       "\t\t\tname = \"ck3_war_0\"\n"
       "\t\t\tordinal = 1\n"
       "\t\t\tfirst = {\n\t\t\t\tname = \"TUN\"\n\t\t\t}\n"
       "\t\t\tsecond = {\n\t\t\t\tname = \"BYZ\"\n\t\t\t}\n"
       "\t\t}\n\n"
       "\t\ttake_province = {\n"
       "\t\t\ttype = conquer_province\n"
       "\t\t\tcasus_belli = cb_conquer_province\n"
       "\t\t\tlocation = palermo\n"
       "\t\t}\n\n"
       "\t\tstart_date = 1336.5.2\n"
       "\t\taction = 1337.3.1\n"
       "\t\tattacker = {\n\t\t\tcountry = TUN\n\t\t\trequest = {\n\t\t\t\treason = Instigator\n\t\t\t}\n\t\t}\n"
       "\t\tattacker = {\n\t\t\tcountry = MLT\n\t\t\trequest = {\n\t\t\t\tcaller = TUN\n\t\t\t\treason = Subject\n\t\t\t}\n\t\t}\n"
       "\t\tattacker = {\n\t\t\tcountry = EGY\n\t\t\trequest = {\n\t\t\t\tcaller = TUN\n\t\t\t\treason = Scripted\n"
       "\t\t\t\twhich = alliance\n\t\t\t}\n\t\t}\n"
       "\t\tdefender = {\n\t\t\tcountry = BYZ\n\t\t\trequest = {\n\t\t\t\treason = Target\n\t\t\t}\n\t\t}\n"
       "\t}\n",
       written);
}

TEST(OutputWarsFileTests, WarsStartBeforeEU5sLastAction)  // NOLINT : clang-tidy doens't like gtest
{
   auto war = MakeWar();
   war.start_date = date("1066.9.28");

   // Moved onto EU5's calendar by whole years, a war started late in the save's year would begin after
   // EU5's own wars last acted, so it starts then instead.
   EXPECT_NE(std::string::npos, WriteWar(war, date("1066.1.1")).find("start_date = 1337.3.1\n"));
}

TEST(OutputWarsFileTests, UnnamedWarsUseEU5sOwnName)  // NOLINT : clang-tidy doens't like gtest
{
   auto war = MakeWar();
   war.name_key.clear();
   war.name.clear();

   const auto written = WriteWar(war, date("867.1.1"));

   EXPECT_NE(std::string::npos, written.find("\twar = {\n"));
   EXPECT_NE(std::string::npos, written.find("name = \"NORMAL_WAR_NAME\""));
}

TEST(OutputWarsFileTests, EveryoneAtWarRaisesTheirLevyAtHome)  // NOLINT : clang-tidy doens't like gtest
{
   std::stringstream definitions;
   definitions << "africa = { maghreb = { ifriqiya_region = { tunis_area = { tunis_province = { tunis carthage } } } } }\n";
   definitions << "europe = { italy = { south_italy_region = { sicily_area = { palermo_province = { palermo } } } } }\n";
   const eu5::MapAreas areas(definitions);
   auto second_war = MakeWar();
   second_war.attackers = {{"EGY", "Instigator", ""}};  // already raised for the first war

   const auto levies =
       WriteLevies({MakeWar(), second_war}, {{"TUN", "tunis"}, {"BYZ", "palermo"}, {"EGY", "cairo"}}, areas);

   EXPECT_EQ(
       "\n\tlevy = {\n\t\tcountry = TUN\n\t\tarea = tunis_area\n\t\tlevy = 1\n\t\tlocation = tunis\n\t}\n"
       "\n\tlevy = {\n\t\tcountry = BYZ\n\t\tarea = sicily_area\n\t\tlevy = 1\n\t\tlocation = palermo\n\t}\n",
       levies);
}

}  // namespace out
