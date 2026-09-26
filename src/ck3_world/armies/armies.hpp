#ifndef CK3_ARMIES_H
#define CK3_ARMIES_H

#include <istream>
#include <map>

namespace ck3
{

// The save's armies section, read for the men-at-arms: each regiment of a type - light_footmen,
// pikemen_unit - belongs to the ruler who raised it. Levies and garrisons carry no type.
class Armies
{
  public:
   Armies() = default;
   explicit Armies(std::istream& input_stream);

   // Ruler character ID to the soldiers in the men-at-arms regiments they own.
   [[nodiscard]] const auto& GetMenAtArms() const { return men_at_arms_; }

  private:
   std::map<long long, int> men_at_arms_;
};

}  // namespace ck3

#endif  // CK3_ARMIES_H
