#ifndef CK3_FAITH_H
#define CK3_FAITH_H
#include <memory>
#include <set>
#include <vector>

#include "Color.h"
#include "Parser.h"
#include "src/ck3_world/id_pointer_pair.hpp"

namespace ck3
{

class Religion;
class Title;
class Faith: commonItems::parser  // NOLINT : issues with error handling in parser
{
  public:
   Faith() = default;
   Faith(std::istream& input_stream, long long faith_id);
   // A rite. CK3 1.20 divides each faith into rites - the Roman and the Byzantine rite of one
   // Christian faith, Hanafi and Maliki of Sunni Islam - and counties and characters belong to a rite.
   // Many rites carry the name their faith had before. A rite is kept as a faith of its own, whose
   // parent is the faith it belongs to; a custom name only for one made in the campaign.
   Faith(long long rite_id, std::string tag, std::string custom_name, long long parent_faith_id);

   [[nodiscard]] const auto& GetTag() const { return tag_; }
   [[nodiscard]] const auto& GetDoctrines() const { return doctrines_; }
   [[nodiscard]] const auto& GetTenets() const { return tenets_; }
   [[nodiscard]] const auto& GetReligion() const { return religion_; }
   [[nodiscard]] const auto& GetReligionHead() const { return religious_head_; }
   [[nodiscard]] auto GetID() const { return faith_id_; }
   [[nodiscard]] const auto& GetCustomName() const { return custom_name_; }
   [[nodiscard]] const auto& GetCustomAdjective() const { return custom_adjective_; }
   [[nodiscard]] const auto& GetDescription() const { return description_; }
   [[nodiscard]] const auto& GetIconPath() const { return icon_path_; }
   [[nodiscard]] const auto& GetFaithType() const { return faith_type_; }
   [[nodiscard]] const auto& IsReformed() const { return is_reformed_; }
   // For a rite, the faith it belongs to; nothing for a faith.
   [[nodiscard]] std::shared_ptr<Faith> GetParentFaith() const;
   // For a faith, its rites, its main one first.
   [[nodiscard]] const auto& GetRites() const { return rites_; }
   [[nodiscard]] auto GetMainRiteID() const { return main_rite_id_; }

   void LinkReligiousHead(const std::map<long long, std::shared_ptr<Title>>& title_map);
   void LinkReligion(const std::map<long long, std::shared_ptr<Religion>>& religion_map);
   // A rite takes its religion from its faith, and is listed among the faith's rites.
   void LinkParentFaith(const std::map<long long, std::shared_ptr<Faith>>& faiths, const std::shared_ptr<Faith>& self);

  private:
   void ParseFaith(std::istream& input_stream);
   void ParseDoctrine(std::istream& input_stream);

   bool is_reformed_ = true;

   long long faith_id_ = -1;
   long long main_rite_id_ = -1;
   std::string tag_;
   std::string faith_type_;
   std::filesystem::path icon_path_;
   std::string custom_name_;
   std::string custom_adjective_;
   std::string description_;
   std::set<std::string> doctrines_;
   std::set<std::string> tenets_;

   // The head of faith is a TITLE (e.g. k_papal_state, d_sunni), not a character.
   std::optional<IdPointerPair<Title>> religious_head_;
   IdPointerPair<Religion> religion_;
   std::optional<IdPointerPair<Faith>> parent_faith_;
   std::vector<std::shared_ptr<Faith>> rites_;
};
}  // namespace ck3

#endif  // CK3_FAITH_H
