#ifndef CK3_RELATIONS_H
#define CK3_RELATIONS_H

#include <Date.h>

#include <istream>
#include <map>
#include <set>
#include <utility>

namespace ck3
{

// The save's relations section: what holds between pairs of characters. Alliances and truces are
// read. Most pairs are courtiers and kin rather than rulers; which alliances matter is decided once
// it is known who rules what.
class Relations
{
  public:
   Relations() = default;
   explicit Relations(std::istream& input_stream);

   // Each allied pair of characters, lower ID first, once.
   [[nodiscard]] const auto& GetAlliances() const { return alliances_; }
   // Each pair of characters under a truce, lower ID first, with the date the last of their truces
   // ends. CK3 keeps one each way (truce_0, truce_1), as it counts who may not attack whom.
   [[nodiscard]] const auto& GetTruces() const { return truces_; }

  private:
   void ParseActiveRelations(std::istream& input_stream);

   std::set<std::pair<long long, long long>> alliances_;
   std::map<std::pair<long long, long long>, date> truces_;
};

}  // namespace ck3

#endif  // CK3_RELATIONS_H
