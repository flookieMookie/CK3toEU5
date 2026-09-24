#include <sstream>

#include "gtest/gtest.h"
#include "src/ck3_world/wars/wars.hpp"

namespace ck3
{

TEST(CK3WorldWarsTests, ActiveWarsAreReadWithTheirSides)  // NOLINT : clang-tidy doens't like gtest
{
   std::stringstream input;
   input << "= {\n";
   input << "\tactive_wars={\n";
   input << "\t\t0={\n";
   input << "\t\t\tattacker={\n";
   input << "\t\t\t\tparticipants={ {\n";
   input << "\t\t\t\t\t\tcharacter=12631\n";
   input << "\t\t\t\t\t\tdate=867.1.1\n";
   input << "\t\t\t\t\t\tcontribution={ 100 0 0 }\n";
   input << "\t\t\t\t\t}\n";
   input << " {\n\t\t\t\t\t\tcharacter=500\n\t\t\t\t\t}\n";
   input << " }\n";
   input << "\t\t\t\tcasualties={\n\t\t\t\t}\n";
   input << "\t\t\t}\n";
   input << "\t\t\tdefender={\n";
   input << "\t\t\t\tparticipants={ {\n\t\t\t\t\t\tcharacter=13582\n\t\t\t\t\t}\n }\n";
   input << "\t\t\t\tcontrols_all=yes\n";
   input << "\t\t\t}\n";
   input << "\t\t\tstart_date=866.5.2\n";
   input << "\t\t\tcasus_belli={\n";
   input << "\t\t\t\ttype=ducal_conquest_cb\t\t\t\tscope={\n\t\t\t\t\troot={\n\t\t\t\t\t\ttype=cb\n\t\t\t\t\t}\n\t\t\t\t}\n";
   input << "\t\t\t\ttargeted_titles={ 2101 }\n";
   input << "\t\t\t\tattacker=12631\n";
   input << "\t\t\t\tdefender=13582\n";
   input << "\t\t\t\tclaimant=4294967295\n";
   input << "\t\t\t}\n";
   input << "\t\t\tname=\"Aghlabid Conquest of Sicily\"\n";
   input << "\t\t}\n";
   input << "\t\t1=none\n";
   input << "\t}\n";
   input << "\tnames={ }\n";
   input << "}\n";

   const Wars wars(input);

   ASSERT_EQ(1, wars.GetWars().size());
   const auto& war = wars.GetWars()[0];
   EXPECT_EQ("Aghlabid Conquest of Sicily", war.name);
   EXPECT_EQ(date("866.5.2"), war.start_date);
   EXPECT_EQ("ducal_conquest_cb", war.casus_belli);
   EXPECT_EQ(12631, war.attacker);
   EXPECT_EQ(13582, war.defender);
   EXPECT_EQ((std::vector<long long>{12631, 500}), war.attackers);
   EXPECT_EQ((std::vector<long long>{13582}), war.defenders);
   EXPECT_EQ((std::vector<long long>{2101}), war.targeted_titles);
}

TEST(CK3WorldWarsTests, TextMarkupIsTakenOutOfNames)  // NOLINT : clang-tidy doens't like gtest
{
   std::stringstream input;
   input << "= { active_wars={ 0={ casus_belli={ attacker=1 defender=2 }\n";
   input << "name=\"\x15ONCLICK:TITLE,3615 \x15TOOLTIP:LANDED_TITLE,3615 \x15L; Holmgar\xC3\xB0rer\x15!\x15!\x15! Conquest of "
            "the \x15ONCLICK:TITLE,3637 \x15TOOLTIP:LANDED_TITLE,3637 \x15L; Chiefdom of Vodi\x15!\x15!\x15!\"\n";
   input << "} 1={ casus_belli={ attacker=3 defender=4 }\n";
   input << "name=\"War for Vimara's \x15" "E; \x15TOOLTIP:GAME_CONCEPT,claim Claim\x15!\x15! on Portucale\"\n";
   input << "} } }\n";

   const Wars wars(input);

   ASSERT_EQ(2, wars.GetWars().size());
   EXPECT_EQ("Holmgar\xC3\xB0rer Conquest of the Chiefdom of Vodi", wars.GetWars()[0].name);
   EXPECT_EQ("War for Vimara's Claim on Portucale", wars.GetWars()[1].name);
}

}  // namespace ck3
