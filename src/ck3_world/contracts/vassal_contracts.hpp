#ifndef CK3_VASSAL_CONTRACTS_H
#define CK3_VASSAL_CONTRACTS_H

#include <istream>
#include <string>
#include <vector>

namespace ck3
{

// One subject contract between two characters, as the save records it.
struct VassalContract
{
   long long vassal_id = 0;
   long long liege_id = 0;
   // feudal_vassal, clan_vassal, tributary_mandala, tributary_subjugated, ...
   std::string group;

   // A tributary pays its suzerain but rules independently, so it is its own realm in CK3 and its
   // own country in EU5 - unlike a vassal, whose land the realm walk already assigns to its liege.
   [[nodiscard]] bool IsTributary() const { return group.starts_with("tributary"); }
};

// The save's vassal_contracts database. The realm walk already knows who is whose vassal from the
// titles' de facto lieges; what only these contracts record is tributaries.
class VassalContracts
{
  public:
   VassalContracts() = default;
   explicit VassalContracts(std::istream& input_stream);

   [[nodiscard]] const auto& GetContracts() const { return contracts_; }
   [[nodiscard]] std::vector<VassalContract> GetTributaries() const;

  private:
   void ParseDatabase(std::istream& input_stream);

   std::vector<VassalContract> contracts_;
};

}  // namespace ck3

#endif  // CK3_VASSAL_CONTRACTS_H
