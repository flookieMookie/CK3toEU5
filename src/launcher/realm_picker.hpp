#ifndef LAUNCHER_REALM_PICKER_H
#define LAUNCHER_REALM_PICKER_H

#include <windows.h>

#include <optional>
#include <string>
#include <vector>

#include "launcher_core.hpp"

namespace launcher
{

// Shows the save's realms with a tick box each, over the launcher's window, and waits for the player.
// Returns the keys of the realms they ticked - none ticked means every realm - or nothing if they
// cancelled. Dark when the launcher is.
[[nodiscard]] std::optional<std::vector<std::string>> PickRealms(HWND owner,
    const std::vector<RealmChoice>& realms,
    const std::vector<std::string>& picked,
    HFONT font,
    bool dark);

}  // namespace launcher

#endif  // LAUNCHER_REALM_PICKER_H
