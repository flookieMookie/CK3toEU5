#include <sstream>

#include "gtest/gtest.h"
#include "src/ck3_world/armies/armies.hpp"

namespace ck3
{

TEST(CK3WorldArmiesTests, MenAtArmsAreCountedForTheirOwner)  // NOLINT : clang-tidy doens't like gtest
{
   std::stringstream input;
   input << "= {\n";
   input << "\tregiments={\n";
   input << "\t\t0={ origin=3003 max=179 }\n";
   input << "\t\t1={ origin=3003 max=584 chunks={ { max=584 current=0 } } source=garrison }\n";
   input << "\t\t12403={ type=light_horsemen size=311 owner=34496 source=hired }\n";
   input << "\t\t12404={ type=light_footmen size=311 owner=34496 source=hired }\n";
   input << "\t\t12405={ size=414 owner=34502 source=hired }\n";
   input << "\t\t12406={ type=pikemen_unit size=311 owner=34502 source=hired }\n";
   input << "\t}\n";
   input << "\tarmy_regiments={ 0={ army=0 source=event } }\n";
   input << "}\n";

   const Armies armies(input);

   ASSERT_EQ(2, armies.GetMenAtArms().size());
   EXPECT_EQ(622, armies.GetMenAtArms().at(34496));
   EXPECT_EQ(311, armies.GetMenAtArms().at(34502));
}

}  // namespace ck3
