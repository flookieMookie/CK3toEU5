#include <sstream>

#include "gtest/gtest.h"
#include "src/ck3_world/characters/characters.hpp"
#include "src/ck3_world/council_manager/councillor_tasks.hpp"
#include "src/ck3_world/realms/realms.hpp"
#include "src/ck3_world/titles/title.hpp"
#include "src/ck3_world/titles/titles.hpp"

namespace ck3
{

TEST(CK3WorldRealmsTests, RealmsDefaultToEmpty)  // NOLINT : clang-tidy doens't like gtest
{
   const Realms realms;

   ASSERT_TRUE(realms.GetRealms().empty());
}

TEST(CK3WorldRealmsTests, OnlyTheIndependentTitleBecomesARealm)  // NOLINT : clang-tidy doens't like gtest
{
   std::stringstream title_input;
   title_input << "landed_titles={\n";
   title_input << "1={key=k_munster holder=100 capital=2}\n";
   title_input << "2={key=c_desmond holder=100 de_facto_liege=1}\n";
   title_input << "3={key=c_ormond holder=200 de_facto_liege=1}\n";
   title_input << "}";
   const Titles titles(title_input);

   std::stringstream character_input;
   character_input << "100={first_name=\"Gerad\" landed_data={domain={ 1 2 } government=\"feudal_government\"}}\n";
   character_input << "200={first_name=\"Donnchad\" landed_data={domain={ 3 }}}\n";
   Characters characters;
   characters.ParseCharacters(character_input);
   const CouncillorTasks councillor_tasks;
   characters.LinkTitles(titles, councillor_tasks);

   const Realms realms(titles, characters);

   // The vassal count is under a held liege, so only the king is a realm.
   ASSERT_EQ(1, realms.GetRealms().size());
   ASSERT_EQ("k_munster", realms.GetRealms().front()->GetPrimaryTitle()->GetKey());
   ASSERT_EQ("Gerad", realms.GetRealms().front()->GetRulerName());
   ASSERT_EQ("feudal_government", realms.GetRealms().front()->GetGovernment());
}

TEST(CK3WorldRealmsTests, RealmGathersDeFactoVassalCounties)  // NOLINT : clang-tidy doens't like gtest
{
   std::stringstream title_input;
   title_input << "landed_titles={\n";
   title_input << "1={key=k_munster holder=100 capital=2}\n";
   title_input << "2={key=c_desmond holder=100 de_facto_liege=1}\n";
   title_input << "3={key=c_ormond holder=200 de_facto_liege=1}\n";
   title_input << "}";
   const Titles titles(title_input);

   std::stringstream character_input;
   character_input << "100={first_name=\"Gerad\" landed_data={domain={ 1 2 }}}\n";
   character_input << "200={first_name=\"Donnchad\" landed_data={domain={ 3 }}}\n";
   Characters characters;
   characters.ParseCharacters(character_input);
   const CouncillorTasks councillor_tasks;
   characters.LinkTitles(titles, councillor_tasks);

   const Realms realms(titles, characters);

   // The king's own county plus the one his vassal count holds beneath him.
   ASSERT_EQ(1, realms.GetRealms().size());
   ASSERT_EQ(2, realms.GetRealms().front()->GetCounties().size());
}

TEST(CK3WorldRealmsTests, CapitalCountyIsResolved)  // NOLINT : clang-tidy doens't like gtest
{
   std::stringstream title_input;
   title_input << "landed_titles={\n";
   title_input << "1={key=k_munster holder=100 capital=2}\n";
   title_input << "2={key=c_desmond holder=100 de_facto_liege=1}\n";
   title_input << "}";
   const Titles titles(title_input);

   std::stringstream character_input;
   character_input << "100={first_name=\"Gerad\" landed_data={domain={ 1 2 }}}\n";
   Characters characters;
   characters.ParseCharacters(character_input);
   const CouncillorTasks councillor_tasks;
   characters.LinkTitles(titles, councillor_tasks);

   const Realms realms(titles, characters);

   ASSERT_NE(nullptr, realms.GetRealms().front()->GetCapitalCounty());
   ASSERT_EQ("c_desmond", realms.GetRealms().front()->GetCapitalCounty()->GetKey());
}

TEST(CK3WorldRealmsTests, TitleWithADestroyedLiegeIsIndependent)  // NOLINT : clang-tidy doens't like gtest
{
   // CK3 leaves destroyed titles in the save with no holder. A vassal of one is really independent.
   std::stringstream title_input;
   title_input << "landed_titles={\n";
   title_input << "1={key=e_gone}\n";
   title_input << "2={key=k_munster holder=100 capital=3 de_facto_liege=1}\n";
   title_input << "3={key=c_desmond holder=100 de_facto_liege=2}\n";
   title_input << "}";
   const Titles titles(title_input);

   std::stringstream character_input;
   character_input << "100={first_name=\"Gerad\" landed_data={domain={ 2 3 }}}\n";
   Characters characters;
   characters.ParseCharacters(character_input);
   const CouncillorTasks councillor_tasks;
   characters.LinkTitles(titles, councillor_tasks);

   const Realms realms(titles, characters);

   ASSERT_EQ(1, realms.GetRealms().size());
   ASSERT_EQ("k_munster", realms.GetRealms().front()->GetPrimaryTitle()->GetKey());
}

TEST(CK3WorldRealmsTests, LandlessTitularTitlesAreNotRealms)  // NOLINT : clang-tidy doens't like gtest
{
   // A mercenary company holds no county or barony, so it is not a realm.
   std::stringstream title_input;
   title_input << "landed_titles={\n";
   title_input << "1={key=d_mercenary_company holder=100}\n";
   title_input << "}";
   const Titles titles(title_input);

   std::stringstream character_input;
   character_input << "100={first_name=\"Sellsword\" landed_data={domain={ 1 }}}\n";
   Characters characters;
   characters.ParseCharacters(character_input);
   const CouncillorTasks councillor_tasks;
   characters.LinkTitles(titles, councillor_tasks);

   const Realms realms(titles, characters);

   ASSERT_TRUE(realms.GetRealms().empty());
}

TEST(CK3WorldRealmsTests, OneRealmPerRulerHoldingSeveralTitles)  // NOLINT : clang-tidy doens't like gtest
{
   std::stringstream title_input;
   title_input << "landed_titles={\n";
   title_input << "1={key=k_munster holder=100 capital=3}\n";
   title_input << "2={key=k_leinster holder=100}\n";
   title_input << "3={key=c_desmond holder=100 de_facto_liege=1}\n";
   title_input << "4={key=c_dublin holder=100 de_facto_liege=2}\n";
   title_input << "}";
   const Titles titles(title_input);

   std::stringstream character_input;
   character_input << "100={first_name=\"Gerad\" landed_data={domain={ 1 2 3 4 }}}\n";
   Characters characters;
   characters.ParseCharacters(character_input);
   const CouncillorTasks councillor_tasks;
   characters.LinkTitles(titles, councillor_tasks);

   const Realms realms(titles, characters);

   ASSERT_EQ(1, realms.GetRealms().size());
   ASSERT_EQ(2, realms.GetRealms().front()->GetHeldTitles().size());
   ASSERT_EQ(2, realms.GetRealms().front()->GetCounties().size());
   // Domain order decides the primary title.
   ASSERT_EQ("k_munster", realms.GetRealms().front()->GetPrimaryTitle()->GetKey());
}

}  // namespace ck3
