#ifndef CK3_HOUSE_H
#define CK3_HOUSE_H
#include "Parser.h"
#include "src/ck3_world/id_pointer_pair.hpp"

namespace ck3
{

class Character;
class Dynasty;
class House: commonItems::parser
{
  public:
   House() = default;
   House(std::istream& input_stream, long long house_id);
   [[nodiscard]] const auto& GetKey() const { return key_; }
   [[nodiscard]] const auto& GetName() const { return name_; }
   [[nodiscard]] const auto& GetLocalizedName() const { return localized_name_; }
   [[nodiscard]] const auto& GetPrefix() const { return prefix_; }
   [[nodiscard]] const auto& GetDynasty() const { return dynasty_; }
   [[nodiscard]] const auto& GetID() const { return house_id_; }
   [[nodiscard]] const auto& GetHouseHead() const { return house_head_; }

   void LinkHouseHead(const std::map<long long, std::shared_ptr<Character>>& characters_map);
   // CK3 1.20 saves its own houses by key alone; their name and prefix are in the game's
   // common/dynasty_houses, and only fill what the save left out.
   void FillFromDefinition(const std::string& name, const std::string& prefix);
   void LinkDynasty(const std::map<long long, std::shared_ptr<Dynasty>>& dynasties_map);

  private:
   void ParseHouse(std::istream& input_stream);

   long long house_id_ = -1;
   std::string key_;
   std::string name_;
   std::string localized_name_;
   IdPointerPair<Dynasty> dynasty_;
   std::string prefix_;
   std::optional<IdPointerPair<Character>> house_head_;  // houses can have missing heads or dead people...
};
}  // namespace ck3

#endif  // CK3_HOUSE_H
