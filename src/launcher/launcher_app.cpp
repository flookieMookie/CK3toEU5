#include <windows.h>

#include "launcher_window.hpp"

int WINAPI wWinMain(_In_ HINSTANCE instance,
    _In_opt_ HINSTANCE /*previous*/,
    _In_ PWSTR /*command_line*/,
    _In_ int show)
{
   return launcher::RunLauncher(instance, show);
}
