#ifndef LAUNCHER_THEME_H
#define LAUNCHER_THEME_H

#include <commctrl.h>
#include <dwmapi.h>
#include <uxtheme.h>
#include <windows.h>

#include <array>

namespace launcher
{

// The colours the launcher draws with, in its light look - Windows' own - or its dark one.
struct Palette
{
   COLORREF window = 0;  // behind everything
   COLORREF field = 0;   // inside text boxes, lists and the log
   COLORREF border = 0;  // the thin lines around things
   COLORREF text = 0;
   COLORREF gray = 0;  // hints and notes
   COLORREF good = 0;
   COLORREF bad = 0;
   COLORREF caution = 0;
   COLORREF link = 0;
   COLORREF progress = 0;  // the bar that fills up
};

// What a piece of text is saying, which decides its colour in either look.
enum class Tone
{
   kNormal,
   kGray,
   kGood,
   kBad,
   kCaution
};

[[nodiscard]] inline Palette PaletteFor(const bool dark)
{
   if (dark)
   {
      return {.window = RGB(32, 32, 32),
          .field = RGB(45, 45, 45),
          .border = RGB(90, 90, 90),
          .text = RGB(232, 232, 232),
          .gray = RGB(160, 160, 160),
          .good = RGB(110, 205, 110),
          .bad = RGB(245, 120, 120),
          .caution = RGB(235, 185, 95),
          .link = RGB(120, 180, 255),
          .progress = RGB(60, 165, 80)};
   }
   return {.window = GetSysColor(COLOR_BTNFACE),
       .field = GetSysColor(COLOR_WINDOW),
       .border = GetSysColor(COLOR_3DSHADOW),
       .text = GetSysColor(COLOR_WINDOWTEXT),
       .gray = GetSysColor(COLOR_GRAYTEXT),
       .good = RGB(0, 128, 0),
       .bad = RGB(192, 0, 0),
       .caution = RGB(170, 100, 0),
       .link = GetSysColor(COLOR_HOTLIGHT),
       .progress = GetSysColor(COLOR_HIGHLIGHT)};
}

[[nodiscard]] inline COLORREF ColourOf(const Palette& palette, const Tone tone)
{
   switch (tone)
   {
      case Tone::kGray:
         return palette.gray;
      case Tone::kGood:
         return palette.good;
      case Tone::kBad:
         return palette.bad;
      case Tone::kCaution:
         return palette.caution;
      default:
         return palette.text;
   }
}

// Whether Windows itself is set to show apps dark, which the launcher follows until told otherwise.
[[nodiscard]] inline bool WindowsPrefersDark()
{
   DWORD light = 1;
   DWORD size = sizeof(light);
   RegGetValueW(HKEY_CURRENT_USER,
       L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
       L"AppsUseLightTheme",
       RRF_RT_REG_DWORD,
       nullptr,
       &light,
       &size);
   return light == 0;
}

// The title bar, which Windows draws: dark to go with a dark window.
inline void UseDarkTitleBar(HWND window, const bool dark)
{
   constexpr DWORD kUseImmersiveDarkMode = 20;  // DWMWA_USE_IMMERSIVE_DARK_MODE
   const BOOL value = dark ? TRUE : FALSE;
   DwmSetWindowAttribute(window, kUseImmersiveDarkMode, &value, sizeof(value));
}

// A group box without a theme has a white, etched frame, which glares on a dark window. In the dark it
// is drawn here instead: a thin frame with the caption set into its top edge.
inline LRESULT CALLBACK DarkGroupBoxProcedure(HWND box,
    const UINT message,
    const WPARAM wparam,
    const LPARAM lparam,
    const UINT_PTR id,
    DWORD_PTR)
{
   if (message == WM_NCDESTROY)
   {
      RemoveWindowSubclass(box, &DarkGroupBoxProcedure, id);
   }
   else if (message == WM_PAINT)
   {
      const auto palette = PaletteFor(true);
      PAINTSTRUCT paint{};
      auto* context = BeginPaint(box, &paint);
      auto* old_font = SelectObject(context, reinterpret_cast<HFONT>(SendMessageW(box, WM_GETFONT, 0, 0)));
      std::array<wchar_t, 128> caption{};
      const auto length = GetWindowTextW(box, caption.data(), static_cast<int>(caption.size()));
      SIZE extent{};
      GetTextExtentPoint32W(context, caption.data(), length, &extent);

      RECT frame{};
      GetClientRect(box, &frame);
      frame.top += extent.cy / 2;
      auto* brush = CreateSolidBrush(palette.border);
      FrameRect(context, &frame, brush);
      DeleteObject(brush);

      // The caption covers the stretch of frame behind it.
      const int gap = extent.cy / 4;
      const RECT label = {2 * gap, 0, 4 * gap + extent.cx, extent.cy};
      SetBkColor(context, palette.window);
      SetTextColor(context, palette.text);
      ExtTextOutW(context, 3 * gap, 0, ETO_OPAQUE, &label, caption.data(), static_cast<UINT>(length), nullptr);
      SelectObject(context, old_font);
      EndPaint(box, &paint);
      return 0;
   }
   return DefSubclassProc(box, message, wparam, lparam);
}

// Gives one of Windows' own controls its dark look, or its usual one back. Buttons, drop-downs, text
// boxes and lists have a dark theme of their own. Tick boxes draw their caption in the theme's black
// whatever they are told, and a progress bar keeps its pale track, so in the dark those go without
// a theme and take the colours they are handed.
inline void ThemeControl(HWND control, const bool dark)
{
   std::array<wchar_t, 64> class_name{};
   GetClassNameW(control, class_name.data(), static_cast<int>(class_name.size()));
   const auto is = [&class_name](const wchar_t* name) {
      return lstrcmpiW(class_name.data(), name) == 0;
   };
   const auto style = GetWindowLongPtrW(control, GWL_STYLE);

   if (is(PROGRESS_CLASSW))
   {
      const auto palette = PaletteFor(dark);
      SetWindowTheme(control, dark ? L"" : nullptr, dark ? L"" : nullptr);
      SendMessageW(control, PBM_SETBKCOLOR, 0, static_cast<LPARAM>(dark ? palette.field : CLR_DEFAULT));
      SendMessageW(control, PBM_SETBARCOLOR, 0, static_cast<LPARAM>(dark ? palette.progress : CLR_DEFAULT));
   }
   else if (is(L"Button") && (style & BS_TYPEMASK) == BS_GROUPBOX)
   {
      if (dark)
      {
         SetWindowSubclass(control, &DarkGroupBoxProcedure, 1, 0);
      }
      else
      {
         RemoveWindowSubclass(control, &DarkGroupBoxProcedure, 1);
      }
   }
   else if (!dark)
   {
      SetWindowTheme(control, nullptr, nullptr);
   }
   else if (is(L"Button"))
   {
      const auto type = style & BS_TYPEMASK;
      const bool tick_box = type == BS_AUTOCHECKBOX || type == BS_CHECKBOX;
      SetWindowTheme(control, tick_box ? L"" : L"DarkMode_Explorer", tick_box ? L"" : nullptr);
   }
   else if (is(L"ComboBox") || is(L"Edit"))
   {
      SetWindowTheme(control, L"DarkMode_CFD", nullptr);
   }
   else if (is(L"RICHEDIT50W") || is(L"SysListView32"))
   {
      SetWindowTheme(control, L"DarkMode_Explorer", nullptr);
   }
   else if (is(L"SysHeader32"))
   {
      SetWindowTheme(control, L"DarkMode_ItemsView", nullptr);
   }
}

}  // namespace launcher

#endif  // LAUNCHER_THEME_H
