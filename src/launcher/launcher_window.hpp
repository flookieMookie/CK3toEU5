#ifndef LAUNCHER_LAUNCHER_WINDOW_H
#define LAUNCHER_LAUNCHER_WINDOW_H

#include <windows.h>

namespace launcher
{

// Shows the launcher's one window and runs it until the player closes it. The player picks a save,
// checks the game folders it found, and presses Convert; the converter runs in the background and
// the finished mod is copied into EU5's mod folder.
//
// Written against Windows itself rather than a toolkit, so the download carries no GUI libraries.
int RunLauncher(HINSTANCE instance, int show);

}  // namespace launcher

#endif  // LAUNCHER_LAUNCHER_WINDOW_H
