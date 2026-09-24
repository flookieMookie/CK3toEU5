#include <sstream>

#include "gtest/gtest.h"
#include "src/ck3_world/contracts/vassal_contracts.hpp"

namespace ck3
{

TEST(CK3WorldVassalContractsTests, ContractsDefaultToEmpty)  // NOLINT : clang-tidy doens't like gtest
{
   const VassalContracts contracts;

   EXPECT_TRUE(contracts.GetContracts().empty());
   EXPECT_TRUE(contracts.GetTributaries().empty());
}

TEST(CK3WorldVassalContractsTests, ContractsAreReadFromTheDatabase)  // NOLINT : clang-tidy doens't like gtest
{
   std::stringstream input;
   input << "database={\n";
   input << "\t16777216={ vassal=40199 liege=11569 date=860.1.1 levels={ 1 } contract_group=tribal_vassal }\n";
   input << "\t2682={ vassal=11189 liege=11351 levels={ 4 0=2 } contract_group=tributary_mandala }\n";
   input << "\t2700={ vassal=500 liege=600 contract_group=tributary_subjugated }\n";
   input << "}\n";
   const VassalContracts contracts(input);

   ASSERT_EQ(3, contracts.GetContracts().size());
   EXPECT_EQ(40199, contracts.GetContracts()[0].vassal_id);
   EXPECT_EQ(11569, contracts.GetContracts()[0].liege_id);
   EXPECT_EQ("tribal_vassal", contracts.GetContracts()[0].group);
   EXPECT_FALSE(contracts.GetContracts()[0].IsTributary());
}

TEST(CK3WorldVassalContractsTests, TributariesAreEveryTributaryGroup)  // NOLINT : clang-tidy doens't like gtest
{
   std::stringstream input;
   input << "database={\n";
   input << "\t1={ vassal=1 liege=2 contract_group=feudal_vassal }\n";
   input << "\t2={ vassal=11189 liege=11351 contract_group=tributary_mandala }\n";
   input << "\t3={ vassal=500 liege=600 contract_group=tributary_subjugated }\n";
   input << "}\n";
   const VassalContracts contracts(input);

   const auto tributaries = contracts.GetTributaries();
   ASSERT_EQ(2, tributaries.size());
   EXPECT_EQ(11189, tributaries[0].vassal_id);
   EXPECT_EQ(11351, tributaries[0].liege_id);
   EXPECT_EQ(500, tributaries[1].vassal_id);
}

TEST(CK3WorldVassalContractsTests, EndedContractsAreSkipped)  // NOLINT : clang-tidy doens't like gtest
{
   std::stringstream input;
   input << "database={\n";
   input << "\t1=none\n";
   input << "\t2={ vassal=3 liege=4 contract_group=tributary_mandala }\n";
   input << "\t5={ contract_group=tributary_mandala }\n";
   input << "}\n";
   const VassalContracts contracts(input);

   ASSERT_EQ(1, contracts.GetContracts().size());
   EXPECT_EQ(3, contracts.GetContracts()[0].vassal_id);
}

}  // namespace ck3
