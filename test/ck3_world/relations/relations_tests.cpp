#include <sstream>
#include <utility>

#include "gtest/gtest.h"
#include "src/ck3_world/relations/relations.hpp"

namespace ck3
{

TEST(CK3WorldRelationsTests, RelationsDefaultToEmpty)  // NOLINT : clang-tidy doens't like gtest
{
   const Relations relations;

   EXPECT_TRUE(relations.GetAlliances().empty());
}

TEST(CK3WorldRelationsTests, OnlyAlliedPairsAreKept)  // NOLINT : clang-tidy doens't like gtest
{
   std::stringstream input;
   input << "active_relations={ {\n";
   input << "\tfirst=12504 second=14679\n";
   input << "\tactive_hook_0={ type=house_head_hook expiration_date=9999.1.1 }\n";
   input << "}\n";
   input << " {\n";
   input << "\tfirst=9661 second=10296\n";
   input << "\talliances={ { allied_through_0=11641 allied_through_1=9661 } }\n";
   input << "}\n";
   input << "}\n";
   const Relations relations(input);

   ASSERT_EQ(1, relations.GetAlliances().size());
   EXPECT_TRUE(relations.GetAlliances().contains(std::pair(9661LL, 10296LL)));
}

TEST(CK3WorldRelationsTests, EachAllianceIsKeptOnceLowerIdFirst)  // NOLINT : clang-tidy doens't like gtest
{
   std::stringstream input;
   input << "active_relations={\n";
   input << " { first=20 second=10 alliances={ { allied_through_0=1 } } }\n";
   input << " { first=10 second=20 alliances={ { allied_through_0=2 } } }\n";
   input << "}\n";
   const Relations relations(input);

   ASSERT_EQ(1, relations.GetAlliances().size());
   EXPECT_TRUE(relations.GetAlliances().contains(std::pair(10LL, 20LL)));
}

TEST(CK3WorldRelationsTests, TrucesAreReadWithTheirLastEnd)  // NOLINT : clang-tidy doens't like gtest
{
   std::stringstream input;
   input << "= {\n";
   input << "\tactive_relations={\n";
   input << "\t\t{\n";
   input << "\t\t\tfirst=14234\n";
   input << "\t\t\tsecond=14233\n";
   input << "\t\t\ttruce_0={\n";
   input << "\t\t\t\tdate=895.3.6\n";
   input << "\t\t\t\tname=\"Conquest of the Chiefdom of Rutog\"\n";
   input << "\t\t\t\tresult=victory\n";
   input << "\t\t\t}\n";
   input << "\t\t\ttruce_1={\n";
   input << "\t\t\t\tdate=893.1.1\n";
   input << "\t\t\t}\n";
   input << "\t\t}\n";
   input << "\t\t{\n";
   input << "\t\t\tfirst=1\n";
   input << "\t\t\tsecond=2\n";
   input << "\t\t\talliances={ { allied_through_0=1 allied_through_1=2 } }\n";
   input << "\t\t}\n";
   input << "\t}\n";
   input << "}\n";

   const Relations relations(input);

   ASSERT_EQ(1, relations.GetTruces().size());
   EXPECT_EQ(date("895.3.6"), relations.GetTruces().at({14233, 14234}));
   EXPECT_EQ(1, relations.GetAlliances().size());
}

}  // namespace ck3
