#ifndef EU5_RULER_TRAITS_H
#define EU5_RULER_TRAITS_H

#include <set>
#include <string>
#include <vector>

namespace eu5
{

struct Abilities
{
   int adm = 0;
   int dip = 0;
   int mil = 0;
};

// The EU5 ruler traits a character's CK3 traits become - brave is bold_fighter, a strategist a
// tactical_genius - at most three of them, personality first. EU5 only accepts a trait its allow rules
// (in_game/common/traits/00_ruler.txt) let the character have: some need a good enough ability, and
// some rule each other out, so a trait that would break them is left off.
[[nodiscard]] std::vector<std::string> RulerTraitsFor(const std::set<std::string>& ck3_traits,
    const Abilities& abilities);

}  // namespace eu5

#endif  // EU5_RULER_TRAITS_H
