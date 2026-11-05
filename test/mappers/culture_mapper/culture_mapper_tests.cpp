#include <sstream>

#include "gtest/gtest.h"
#include "src/mappers/culture_mapper/culture_mapper.hpp"

namespace mappers
{

TEST(MappersCultureMapperTests, MappingsDefaultToEmpty)  // NOLINT : clang-tidy doens't like gtest
{
   std::stringstream input;
   const CultureMapper mapper(input);

   EXPECT_TRUE(mapper.GetMappings().empty());
   EXPECT_FALSE(mapper.GetEU5Culture("slovien").has_value());
}

TEST(MappersCultureMapperTests, CK3CulturesMapToTheirEU5Name)  // NOLINT : clang-tidy doens't like gtest
{
   std::stringstream input;
   input << "link = { eu5 = slovak ck3 = slovien }\n";
   input << "link = { eu5 = andalusi ck3 = andalusian ck3 = mozarabic } # several ck3 to one eu5\n";
   input << "link = { ck3 = nowhere }\n";
   const CultureMapper mapper(input);

   EXPECT_EQ("slovak", mapper.GetEU5Culture("slovien"));
   EXPECT_EQ("andalusi", mapper.GetEU5Culture("andalusian"));
   EXPECT_EQ("andalusi", mapper.GetEU5Culture("mozarabic"));
   // A link without an EU5 culture maps nothing.
   EXPECT_FALSE(mapper.GetEU5Culture("nowhere").has_value());
}

}  // namespace mappers
