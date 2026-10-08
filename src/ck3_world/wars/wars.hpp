#ifndef CK3_WARS_H
#define CK3_WARS_H

#include <Date.h>

#include <istream>
#include <string>
#include <vector>

namespace ck3
{

// CK3 renders the names it saves with its text markup in them, each code opened by \x15: a link -
// \x15ONCLICK:TITLE,3615 - or tooltip up to the next space, a style - \x15L; - and \x15! to close
// one. What is left is the name the player read.
[[nodiscard]] std::string WithoutMarkup(const std::string& text);

// One of the save's active wars. Sides are lists of characters, the rulers who joined them.
struct War
{
   std::string name;  // as the save renders it - "Aghlabid Conquest of Sicily"
   date start_date = date("1.1.1");
   std::string casus_belli;
   // The war leaders, as the casus belli names them.
   long long attacker = 0;
   long long defender = 0;
   std::vector<long long> attackers;
   std::vector<long long> defenders;
   // The titles fought over, by save ID.
   std::vector<long long> targeted_titles;
};

// The save's wars section. Only the active ones are read: those that ended left nothing behind
// but truces, which the save keeps elsewhere.
class Wars
{
  public:
   Wars() = default;
   explicit Wars(std::istream& input_stream);

   [[nodiscard]] const auto& GetWars() const { return wars_; }

  private:
   void ParseActiveWars(std::istream& input_stream);

   std::vector<War> wars_;
};

}  // namespace ck3

#endif  // CK3_WARS_H
