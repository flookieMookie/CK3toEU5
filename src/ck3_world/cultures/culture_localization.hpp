#ifndef CK3_CULTURE_LOCALIZATION_H
#define CK3_CULTURE_LOCALIZATION_H

#include <external/commonItems/Localization/LocalizationDatabase.h>

#include <filesystem>
#include <vector>

#include "ModLoader/Mod.h"

namespace ck3
{

// The names CK3 gives its cultures, in every language CK3 ships, so a culture EU5 has to be taught
// can be called what CK3 calls it. Only the culture files are read: CK3's full localisation runs to
// hundreds of megabytes, and the converter needs a few hundred keys of it. The save's mods are read
// after it, so their names win: every file of theirs whose path mentions cultures.
[[nodiscard]] commonItems::LocalizationDatabase LoadCultureLocalization(const std::filesystem::path& ck3_directory,
    const std::vector<Mod>& mods);

// The names CK3 gives its dynasties and houses - dynn_Karling is "Karling" - in the same way.
[[nodiscard]] commonItems::LocalizationDatabase LoadDynastyLocalization(const std::filesystem::path& ck3_directory,
    const std::vector<Mod>& mods);

// CK3's nicknames - nick_the_great is "the Great" - in the same way.
[[nodiscard]] commonItems::LocalizationDatabase LoadNicknameLocalization(const std::filesystem::path& ck3_directory,
    const std::vector<Mod>& mods);

}  // namespace ck3

#endif  // CK3_CULTURE_LOCALIZATION_H
