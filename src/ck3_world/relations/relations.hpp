#ifndef CK3_RELATIONS_H
#define CK3_RELATIONS_H

#include <istream>
#include <set>
#include <utility>

namespace ck3
{

// The save's relations section: what holds between pairs of characters. Only alliances are read so
// far. Most pairs are courtiers and kin rather than rulers; which alliances matter is decided once
// it is known who rules what.
class Relations
{
  public:
   Relations() = default;
   explicit Relations(std::istream& input_stream);

   // Each allied pair of characters, lower ID first, once.
   [[nodiscard]] const auto& GetAlliances() const { return alliances_; }

  private:
   void ParseActiveRelations(std::istream& input_stream);

   std::set<std::pair<long long, long long>> alliances_;
};

}  // namespace ck3

#endif  // CK3_RELATIONS_H
