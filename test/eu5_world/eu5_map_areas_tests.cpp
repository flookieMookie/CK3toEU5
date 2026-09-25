#include <sstream>

#include "gtest/gtest.h"
#include "src/eu5_world/eu5_map_areas.hpp"

namespace eu5
{

TEST(EU5WorldMapAreasTests, LocationsAreInTheirArea)  // NOLINT : clang-tidy doens't like gtest
{
   std::stringstream input;
   input << "\xEF\xBB\xBF" << "europe = {\n";
   input << "\twestern_europe = {\n";
   input << "\t\tscandinavian_region = {\n";
   input << "\t\t\tsvealand_area = {\n";
   input << "\t\t\t\tuppland_province = { stockholm norrtalje } # the capital\n";
   input << "\t\t\t\tnerike_province = { orebro }\n";
   input << "\t\t\t}\n";
   input << "\t\t\tgotaland_area = {\n";
   input << "\t\t\t\tgotland_province = { slite visby\thoborg\t}\n";
   input << "\t\t\t}\n";
   input << "\t\t}\n";
   input << "\t}\n";
   input << "}\n";

   const MapAreas areas(input);

   EXPECT_EQ(6, areas.size());
   EXPECT_EQ("svealand_area", areas.AreaOf("stockholm"));
   EXPECT_EQ("svealand_area", areas.AreaOf("orebro"));
   EXPECT_EQ("gotaland_area", areas.AreaOf("hoborg"));
   EXPECT_EQ("scandinavian_region", areas.RegionOf("hoborg"));
   EXPECT_FALSE(areas.AreaOf("uppland_province").has_value());
   EXPECT_FALSE(areas.AreaOf("the").has_value());
}

}  // namespace eu5
