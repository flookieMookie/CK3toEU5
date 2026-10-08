#include <set>
#include <sstream>
#include <string>
#include <utility>

#include "gtest/gtest.h"
#include "src/ck3_world/relations/opinions.hpp"

namespace ck3
{

TEST(CK3WorldOpinionsTests, OpinionsDefaultToEmpty)  // NOLINT : clang-tidy doens't like gtest
{
   const Opinions opinions;

   EXPECT_TRUE(opinions.GetScriptedRelations().empty());
}

TEST(CK3WorldOpinionsTests, ScriptedRelationsAreReadFromEachSide)  // NOLINT : clang-tidy doens't like gtest
{
   std::stringstream input;
   input << "active_opinions={ {\n";
   input << "\t\towner=14250\n";
   input << "\t\ttarget=14162\n";
   input << "\t\ttemporary_opinion={ modifier=declared_war start_date=867.1.1 value=-25 }\n";
   input << "\t\tscripted_relations={ rival={ flags=\"AA==\" reason=\"rival_historical\" } }\n";
   input << "\t}\n";
   input << " {\n";
   input << "\t\towner=14162 target=14250\n";
   input << "\t\tscripted_relations={ rival={ flags=\"AA==\" } nemesis={ flags=\"AA==\" } }\n";
   input << "\t}\n";
   input << "}\n";
   const Opinions opinions(input);

   ASSERT_EQ(2, opinions.GetScriptedRelations().size());
   EXPECT_EQ(std::set<std::string>{"rival"}, opinions.GetScriptedRelations().at(std::pair(14250LL, 14162LL)));
   EXPECT_EQ((std::set<std::string>{"nemesis", "rival"}),
       opinions.GetScriptedRelations().at(std::pair(14162LL, 14250LL)));
}

TEST(CK3WorldOpinionsTests, PassingOpinionsAloneAreNotKept)  // NOLINT : clang-tidy doens't like gtest
{
   std::stringstream input;
   input << "active_opinions={ {\n";
   input << "\towner=13582 target=12631\n";
   input << "\ttemporary_opinion={ modifier=declared_war start_date=867.1.1 value=-25 }\n";
   input << "} }\n";
   const Opinions opinions(input);

   EXPECT_TRUE(opinions.GetScriptedRelations().empty());
}

}  // namespace ck3
