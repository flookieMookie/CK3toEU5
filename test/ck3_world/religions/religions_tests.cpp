#include <sstream>
#include <stdexcept>

#include "gtest/gtest.h"
#include "src/ck3_world/religions/religions.hpp"
#include "src/ck3_world/titles/title.hpp"
#include "src/ck3_world/titles/titles.hpp"

namespace ck3
{

TEST(CK3WorldReligionsTests, ReligionsDefaultToEmpty)  // NOLINT : clang-tidy doens't like gtest
{
   std::stringstream input;
   const Religions religions(input);

   ASSERT_TRUE(religions.GetReligions().empty());
}

TEST(CK3WorldReligionsTests, BundledReligionsCanBeLoaded)  // NOLINT : clang-tidy doens't like gtest
{
   std::stringstream input;
   input << "religions={\n";
   input << "\t13={tag=\"bon_religion\"}\n";
   input << "\t15={tag=\"buddhism_religion\"}\n";
   input << "}";

   const Religions religions(input);
   const auto& religion1 = religions.GetReligions().find(13);  // NOLINT(readability-magic-numbers) : "magic number"
   const auto& religion2 = religions.GetReligions().find(15);  // NOLINT(readability-magic-numbers) : "magic number"

   ASSERT_EQ(2, religions.GetReligions().size());
   ASSERT_EQ("bon_religion", religion1->second->GetTag());
   ASSERT_EQ("buddhism_religion", religion2->second->GetTag());
}

TEST(CK3WorldFaithsTests, FaithsDefaultToEmpty)  // NOLINT : clang-tidy doens't like gtest
{
   std::stringstream input;
   const Religions religions(input);

   ASSERT_TRUE(religions.GetFaiths().empty());
}

TEST(CK3WorldFaithsTests, FaithsCanBeLoaded)  // NOLINT : clang-tidy doens't like gtest
{
   std::stringstream input;
   input << "faiths={\n";
   input << "13={tag=\"old_bon\"}\n";
   input << "15={tag=\"theravada\"}\n";
   input << "}";

   const Religions religions(input);
   const auto& faith1 = religions.GetFaiths().find(13);  // NOLINT(readability-magic-numbers) : "magic number"
   const auto& faith2 = religions.GetFaiths().find(15);  // NOLINT(readability-magic-numbers) : "magic number"

   ASSERT_EQ("old_bon", faith1->second->GetTag());
   ASSERT_EQ("theravada", faith2->second->GetTag());
}

TEST(CK3WorldFaithsTests, FaithReligiousHeadTitlesCanBeLinked)  // NOLINT : clang-tidy doens't like gtest
{
   std::stringstream title_input;
   title_input << "landed_titles={\n";
   title_input << "0={key=k_papal_state holder=21}\n";  // ID 0 is a real title, not a "no head" sentinel
   title_input << "12={key=d_sunni holder=22}\n";
   title_input << "}";
   const Titles titles(title_input);

   std::stringstream input;
   input << "faiths={\n";
   input << "13={tag=\"catholic\" religious_head=0}\n";
   input << "15={tag=\"ashari\" religious_head=12}\n";
   input << "}";
   Religions religions(input);

   religions.LinkTitles(titles);

   const auto& faith1 = religions.GetFaiths().find(13);  // NOLINT(readability-magic-numbers) : "magic number"
   const auto& faith2 = religions.GetFaiths().find(15);  // NOLINT(readability-magic-numbers) : "magic number"

   ASSERT_EQ("k_papal_state",
       faith1  // NOLINT(bugprone-unchecked-optional-access)
           ->second->GetReligionHead()
           ->GetPointer()
           .lock()
           ->GetKey());
   ASSERT_EQ("d_sunni",
       faith2  // NOLINT(bugprone-unchecked-optional-access)
           ->second->GetReligionHead()
           ->GetPointer()
           .lock()
           ->GetKey());
}

TEST(CK3WorldFaithsTests, LinkingMissingReligiousHeadLeavesItUnlinked)  // NOLINT : clang-tidy doens't like gtest
{
   std::stringstream title_input;
   title_input << "landed_titles={\n";
   title_input << "12={key=d_sunni holder=22}\n";
   title_input << "}";
   const Titles titles(title_input);

   std::stringstream input;
   input << "faiths={\n";
   input << "13={tag=\"ashari\" religious_head=12}\n";
   input << "15={tag=\"theravada\" religious_head=6}\n";  // 6 is missing
   input << "}";
   Religions religions(input);

   ASSERT_NO_THROW(religions.LinkTitles(titles));  // NOLINT : clang-tidy doens't like gtest

   const auto& linked = religions.GetFaiths().find(13);    // NOLINT(readability-magic-numbers) : "magic number"
   const auto& unlinked = religions.GetFaiths().find(15);  // NOLINT(readability-magic-numbers) : "magic number"

   ASSERT_EQ("d_sunni",
       linked  // NOLINT(bugprone-unchecked-optional-access)
           ->second->GetReligionHead()
           ->GetPointer()
           .lock()
           ->GetKey());
   ASSERT_TRUE(unlinked  // NOLINT(bugprone-unchecked-optional-access)
           ->second->GetReligionHead()
           ->GetPointer()
           .expired());
}

TEST(CK3WorldFaithsTests, ReligionsCanBeLinked)  // NOLINT : clang-tidy doens't like gtest
{
   std::stringstream input;
   input << "religions={\n";
   input << "\t11={tag=\"bon_religion\" faiths={ 13 15 }}\n";
   input << "}";
   input << "faiths={\n";
   input << "13={tag=\"old_bon\" religion=11}\n";
   input << "15={tag=\"theravada\" religion=11}\n";
   input << "}";
   Religions religions(input);

   religions.LinkReligions();

   const auto& faith1 = religions.GetFaiths().find(13);        // NOLINT(readability-magic-numbers) : "magic number"
   const auto& faith2 = religions.GetFaiths().find(15);        // NOLINT(readability-magic-numbers) : "magic number"
   const auto& religion1 = religions.GetReligions().find(11);  // NOLINT(readability-magic-numbers) : "magic number"

   ASSERT_EQ("bon_religion", faith1->second->GetReligion().GetPointer().lock()->GetTag());
   ASSERT_EQ("bon_religion", faith2->second->GetReligion().GetPointer().lock()->GetTag());

   ASSERT_EQ("old_bon", religion1->second->GetFaiths()[0].GetPointer().lock()->GetTag());
   ASSERT_EQ("theravada", religion1->second->GetFaiths()[1].GetPointer().lock()->GetTag());
}

TEST(CK3WorldFaithsTests, LinkingMissingReligionIsIgnored)  // NOLINT : clang-tidy doens't like gtest
{
   std::stringstream input;
   input << "religions={\n";
   input << "\t11={tag=\"bon_religion\" faiths={ 13 15 }}\n";
   input << "}";
   input << "faiths={\n";
   input << "13={tag=\"old_bon\" religion=6}\n";  // 6 is missing
   input << "15={tag=\"theravada\" religion=11}\n";
   input << "}";
   Religions religions(input);

   EXPECT_NO_THROW(religions.LinkReligions());  // NOLINT : clang-tidy doens't like gtest
}

TEST(CK3WorldFaithsTests, LinkingMissingFaithIsIgnored)  // NOLINT : clang-tidy doens't like gtest
{
   std::stringstream input;
   input << "religions={\n";
   input << "\t11={tag=\"bon_religion\" faiths={ 13 15 6 }}\n";  // 6 is missing
   input << "}";
   input << "faiths={\n";
   input << "13={tag=\"old_bon\" religion=11}\n";
   input << "15={tag=\"theravada\" religion=11}\n";
   input << "}";
   Religions religions(input);

   EXPECT_NO_THROW(religions.LinkReligions());  // NOLINT : clang-tidy doens't like gtest
}

// CK3 1.20 keeps faiths in a block of their own and divides them into rites, which counties and
// characters belong to.
TEST(CK3WorldReligionsTests, RitesOfCK3_1_20BelongToTheirFaith)  // NOLINT : clang-tidy doens't like gtest
{
   Religions religions;
   std::stringstream religion_input;
   religion_input << "religions={ 7={ tag=\"christianity_religion\" faiths={ 12 13 } } }";
   religions.ParseReligions(religion_input);

   std::stringstream faith_input;
   faith_input << "database={\n";
   faith_input << "\t12={ faith_type=\"christian_faith\" religion=7 main_rite=0 tag=\"christian_faith\" }\n";
   faith_input << "\t13={ faith_type=\"catholic\" religion=7 main_rite=150 tag=\"catholic\" }\n";
   faith_input << "}";
   religions.ParseFaithDatabase(faith_input);

   std::stringstream rite_input;
   rite_input << "database={\n";
   rite_input << "\t6={ rite_type=\"byzantine_rite\" faith=12 data={ tag=\"byzantine_rite\" name=\"Byzantine Rite\" } "
                 "source=historical }\n";
   rite_input << "\t0={ rite_type=\"roman_rite\" faith=12 data={ tag=\"roman_rite\" name=\"Roman Rite\" } "
                 "source=historical }\n";
   rite_input
       << "\t200={ rite_type=\"dynamic_rite_200\" faith=12 data={ name=\"Rite of Saint Maria\" } source=player }\n";
   rite_input << "}";
   religions.ParseRites(rite_input);
   religions.LinkReligions();

   ASSERT_EQ(2, religions.GetFaiths().size());
   ASSERT_EQ(3, religions.GetRites().size());
   const auto& byzantine = religions.GetRites().at(6);
   EXPECT_EQ("byzantine_rite", byzantine->GetTag());
   ASSERT_NE(nullptr, byzantine->GetParentFaith());
   EXPECT_EQ("christian_faith", byzantine->GetParentFaith()->GetTag());
   // A rite takes its faith's religion.
   ASSERT_NE(nullptr, byzantine->GetReligion().GetPointer().lock());
   EXPECT_EQ("christianity_religion", byzantine->GetReligion().GetPointer().lock()->GetTag());
   // A faith lists its rites, its main one first.
   const auto& rites = religions.GetFaiths().at(12)->GetRites();
   ASSERT_EQ(3, rites.size());
   EXPECT_EQ("roman_rite", rites.front()->GetTag());
   // CK3's own rites are named in its localisation; one made in the campaign keeps its own name.
   EXPECT_TRUE(byzantine->GetCustomName().empty());
   EXPECT_EQ("Rite of Saint Maria", religions.GetRites().at(200)->GetCustomName());
   EXPECT_EQ("dynamic_rite_200", religions.GetRites().at(200)->GetTag());
}

}  // namespace ck3