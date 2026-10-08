#include <sstream>

#include "gtest/gtest.h"
#include "src/ck3_world/artifacts/artifacts.hpp"

namespace ck3
{

TEST(CK3WorldArtifactsTests, ArtifactsDefaultToEmpty)  // NOLINT : clang-tidy doens't like gtest
{
   const Artifacts artifacts;

   EXPECT_TRUE(artifacts.GetArtifacts().empty());
}

TEST(CK3WorldArtifactsTests, ArtifactsAreReadWithTheirStory)  // NOLINT : clang-tidy doens't like gtest
{
   std::stringstream input;
   input << "artifacts={\n";
   input << "\t0={\n";
   input << "\t\tname=\"Magnificent Sword\"\n";
   input << "\t\tdescription=\"Owned by \x15ONCLICK:CHARACTER,15143 Master Henry\x15!\"\n";
   input << "\t\ttype=sword\n";
   input
       << "\t\thistory={ entries={ { type=inherited date=870.3.1 } { type=created_before_history date=866.1.1 } } }\n";
   input << "\t\trarity=illustrious\n";
   input << "\t\towner=15143\n";
   input << "\t\tvisuals={ type=sword icon=\"artifact_sword.dds\" }\n";
   input << "\t}\n";
   input << "\t1=none\n";
   input << "\t2={ name=\"\x15L;Papal Tiara\x15!\" type=helmet rarity=famed owner=10027 }\n";
   input << "\t3={ name=\"Lost\" type=book rarity=famed }\n";
   input << "}\n";
   const Artifacts artifacts(input);

   ASSERT_EQ(2, artifacts.GetArtifacts().size());
   const auto& sword = artifacts.GetArtifacts()[0];
   EXPECT_EQ(0, sword.id);
   EXPECT_EQ("Magnificent Sword", sword.name);
   EXPECT_EQ("sword", sword.type);
   EXPECT_EQ("illustrious", sword.rarity);
   EXPECT_EQ(15143, sword.owner);
   ASSERT_TRUE(sword.created.has_value());
   EXPECT_EQ(date("866.1.1"), *sword.created);
   const auto& tiara = artifacts.GetArtifacts()[1];
   EXPECT_EQ(2, tiara.id);
   EXPECT_EQ("Papal Tiara", tiara.name);
   EXPECT_FALSE(tiara.created.has_value());
}

}  // namespace ck3
