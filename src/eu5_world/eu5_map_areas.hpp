#ifndef EU5_MAP_AREAS_H
#define EU5_MAP_AREAS_H

#include <filesystem>
#include <istream>
#include <map>
#include <optional>
#include <string>

namespace eu5
{

// Which area each EU5 location lies in, from in_game/map_data/definitions.txt, where the map nests
// continent > subcontinent > region > area > province = { locations }.
class MapAreas
{
  public:
   MapAreas() = default;
   explicit MapAreas(const std::filesystem::path& eu5_directory);
   // For tests: definitions from an already open file.
   explicit MapAreas(std::istream& input_stream);

   [[nodiscard]] std::optional<std::string> AreaOf(const std::string& location) const;
   [[nodiscard]] auto size() const { return area_of_location_.size(); }

  private:
   void Parse(std::istream& input_stream);

   std::map<std::string, std::string> area_of_location_;
};

}  // namespace eu5

#endif  // EU5_MAP_AREAS_H
