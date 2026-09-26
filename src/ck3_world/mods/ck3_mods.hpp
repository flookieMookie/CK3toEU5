#ifndef CK3_MODS_H
#define CK3_MODS_H

#include <filesystem>
#include <string>
#include <vector>

#include "ModLoader/Mod.h"
#include "ModLoader/ModFilesystem.h"

namespace ck3
{

// The mods a save used - it lists their descriptors, "mod/ugc_2834284159.mod" - found among those
// installed under CK3's documents folder, in load order. Mods that are no longer installed are
// warned about and left out.
[[nodiscard]] std::vector<Mod> ResolveMods(const std::filesystem::path& ck3_documents_directory,
    const std::vector<std::string>& mod_descriptors);

// Whether a mod ships a map of its own. The province mappings are written for CK3's map, so a
// conversion of such a save puts land in the wrong places or nowhere.
[[nodiscard]] bool ChangesMap(const Mod& mod);

// CK3's game files - rooted at <install>/game - with the mods laid over them.
[[nodiscard]] commonItems::ModFilesystem CK3Files(const std::filesystem::path& ck3_directory,
    const std::vector<Mod>& mods);

}  // namespace ck3

#endif  // CK3_MODS_H
