#ifndef OUT_DIPLOMACY_FILE_H
#define OUT_DIPLOMACY_FILE_H

#include <string>
#include <utility>
#include <vector>

#include "src/eu5_world/eu5_world.hpp"

namespace out
{

// Entries for EU5's 12_diplomacy: CK3's vassals and tributaries as EU5 subjects, and the alliances
// between independent rulers, among the countries written to the mod.
[[nodiscard]] std::string WriteSubjectsAndAlliances(const eu5::EU5World& eu5_world);

// Entries for EU5's 20_rivals: each first country rivals the second.
[[nodiscard]] std::string WriteRivals(const std::vector<std::pair<std::string, std::string>>& rivals);

// Entries for EU5's 18_opinions: each first country thinks well of the second, as EU5's own
// countries with good relations do.
[[nodiscard]] std::string WriteGoodRelations(const std::vector<std::pair<std::string, std::string>>& relations);

}  // namespace out

#endif  // OUT_DIPLOMACY_FILE_H
