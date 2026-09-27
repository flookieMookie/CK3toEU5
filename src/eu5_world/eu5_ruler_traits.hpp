#ifndef EU5_RULER_TRAITS_H
#define EU5_RULER_TRAITS_H

#include <Date.h>

#include <cstddef>
#include <optional>
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
    const Abilities& abilities,
    std::size_t most_traits = 3);

// When a converted ruler's reign began on EU5's calendar: the day they took their CK3 title, moved by
// the same years as their birthday, and no later than EU5's start. Empty for a title never held.
[[nodiscard]] std::optional<date> ReignStart(const date& took_title, const date& conversion_date);

// How many ruler traits EU5 lets a ruler have, by the years they have reigned: one after a year, two
// after ten, three after twenty-five (YEARS_OF_RULING_*_TRAIT). EU5 strips any beyond that.
[[nodiscard]] std::size_t RulerTraitSlots(const std::optional<date>& reign_start);

}  // namespace eu5

#endif  // EU5_RULER_TRAITS_H
