#ifndef CK3_OPINIONS_H
#define CK3_OPINIONS_H

#include <istream>
#include <map>
#include <set>
#include <string>
#include <utility>

namespace ck3
{

// The save's opinions section: what each character thinks of another. Only the relations CK3
// scripts between them - rival, nemesis, friend, lover and the like - are read; the passing opinion
// modifiers have no lasting EU5 counterpart.
class Opinions
{
  public:
   Opinions() = default;
   explicit Opinions(std::istream& input_stream);

   // Each owner and target character, with the scripted relations the owner holds towards the target.
   // CK3 records them from each side, so a mutual friendship appears once in each direction.
   [[nodiscard]] const auto& GetScriptedRelations() const { return scripted_relations_; }

  private:
   std::map<std::pair<long long, long long>, std::set<std::string>> scripted_relations_;
};

}  // namespace ck3

#endif  // CK3_OPINIONS_H
