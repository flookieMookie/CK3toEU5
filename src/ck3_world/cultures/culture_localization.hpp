#ifndef CK3_CULTURE_LOCALIZATION_H
#define CK3_CULTURE_LOCALIZATION_H

#include <external/commonItems/Localization/LocalizationDatabase.h>

#include <filesystem>

namespace ck3
{

// The names CK3 gives its cultures, in every language CK3 ships, so a culture EU5 has to be taught
// can be called what CK3 calls it. Only the culture files are read: CK3's full localisation runs to
// hundreds of megabytes, and the converter needs a few hundred keys of it.
[[nodiscard]] commonItems::LocalizationDatabase LoadCultureLocalization(const std::filesystem::path& ck3_directory);

}  // namespace ck3

#endif  // CK3_CULTURE_LOCALIZATION_H
