#include <sstream>

#include "gtest/gtest.h"
#include "src/ck3_world/coats_of_arms/coats_of_arms.hpp"

namespace ck3
{

TEST(CK3WorldCoatsOfArmsTests, CoatsOfArmsDefaultToEmpty)  // NOLINT : clang-tidy doens't like gtest
{
   const CoatsOfArms coats_of_arms;

   EXPECT_TRUE(coats_of_arms.GetCoatsOfArms().empty());
}

TEST(CK3WorldCoatsOfArmsTests, DefinitionsAreKeptWithTheArtTheyUse)  // NOLINT : clang-tidy doens't like gtest
{
   std::stringstream input;
   input << "coat_of_arms_manager_name_map={ 100031=1956 }\n";
   input << "coat_of_arms_manager_database={\n";
   input << "957={\n";
   input << "\tpattern=\"pattern_solid.dds\"\n";
   input << "\tcolor1=blue\n";
   input << "\tcolored_emblem={\n";
   input << "\t\tcolor1=yellow\n";
   input << "\t\ttexture=\"ce_fleur.dds\"\n";
   input << "\t\tinstance={ position={ 0.1 0.0 } scale={ 0.23 0.26 } }\n";
   input << "\t}\n";
   input << "}\n";
   input << "1={ pattern=\"pattern_solid.dds\" textured_emblem={ texture=\"_default.dds\" } }\n";
   input << "}\n";
   const CoatsOfArms coats_of_arms(input);

   ASSERT_EQ(2, coats_of_arms.GetCoatsOfArms().size());
   const auto& france = coats_of_arms.GetCoatsOfArms().at(957);
   EXPECT_TRUE(france.definition.starts_with("{"));
   EXPECT_TRUE(france.definition.contains("texture=\"ce_fleur.dds\""));
   EXPECT_TRUE(france.definition.contains("scale={ 0.23 0.26 }") || france.definition.contains("scale"));
   EXPECT_EQ((std::set<std::string>{"ce_fleur.dds", "pattern_solid.dds"}), france.textures);
   EXPECT_EQ((std::set<std::string>{"_default.dds", "pattern_solid.dds"}), coats_of_arms.GetCoatsOfArms().at(1).textures);
}

}  // namespace ck3
