#ifndef MAPPERS_CULTURE_MAPPER_H
#define MAPPERS_CULTURE_MAPPER_H

#include <filesystem>
#include <istream>
#include <map>
#include <optional>
#include <string>

namespace mappers
{

// The EU5 culture a CK3 culture is, where EU5 calls it something else - CK3's Slovien are EU5's
// Slovaks - from data/configurables/culture_map.txt. One eu5 culture per link, one or more ck3:
//    link = { eu5 = slovak ck3 = slovien }
// A CK3 culture EU5 calls by the same name needs no link.
class CultureMapper
{
  public:
   CultureMapper() = default;
   explicit CultureMapper(std::istream& input_stream);
   explicit CultureMapper(const std::filesystem::path& file_path);

   [[nodiscard]] std::optional<std::string> GetEU5Culture(const std::string& ck3_culture) const;
   [[nodiscard]] const auto& GetMappings() const { return ck3_to_eu5_; }

  private:
   void ParseMappings(std::istream& input_stream);

   std::map<std::string, std::string> ck3_to_eu5_;
};

}  // namespace mappers

#endif  // MAPPERS_CULTURE_MAPPER_H
