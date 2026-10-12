#include "realm_picker.hpp"

#include <commctrl.h>

#include <algorithm>
#include <set>

#include "theme.hpp"
#include "wide_text.hpp"

namespace
{
const wchar_t* const kPickerClass = L"CK3toEU5RealmPicker";
constexpr int kTickAllId = 200;
constexpr int kClearId = 201;

// "3 kings, 41 dukes, 120 counts", leaving out what a realm has none of.
std::wstring DescribeVassals(const launcher::RealmChoice& realm)
{
   std::wstring text;
   const auto add = [&text](const int count, const wchar_t* one, const wchar_t* many) {
      if (count > 0)
      {
         text += (text.empty() ? L"" : L", ") + std::to_wstring(count) + L" " + (count == 1 ? one : many);
      }
   };
   add(realm.kings, L"king", L"kings");
   add(realm.dukes, L"duke", L"dukes");
   add(realm.counts, L"count", L"counts");
   return text;
}

struct Picker
{
   HWND owner = nullptr;
   HWND window = nullptr;
   HWND hint = nullptr;
   HWND list = nullptr;
   HWND tick_all = nullptr;
   HWND clear = nullptr;
   HWND ok = nullptr;
   HWND cancel = nullptr;
   UINT dpi = 96;
   launcher::Palette palette;
   HBRUSH background = nullptr;
   const std::vector<launcher::RealmChoice>* realms = nullptr;
   std::vector<std::string> chosen;
   bool accepted = false;
   bool done = false;

   [[nodiscard]] int Scale(const int pixels) const { return MulDiv(pixels, static_cast<int>(dpi), 96); }

   void Layout() const
   {
      RECT client{};
      GetClientRect(window, &client);
      const int margin = Scale(12);
      const int button_width = Scale(96);
      const int button_height = Scale(28);
      const int hint_height = Scale(36);
      const int buttons_top = client.bottom - margin - button_height;
      MoveWindow(hint, margin, margin, client.right - 2 * margin, hint_height, TRUE);
      MoveWindow(list,
          margin,
          margin + hint_height + Scale(4),
          client.right - 2 * margin,
          std::max(buttons_top - Scale(8) - (margin + hint_height + Scale(4)), 0),
          TRUE);
      MoveWindow(tick_all, margin, buttons_top, button_width, button_height, TRUE);
      MoveWindow(clear, margin + button_width + Scale(8), buttons_top, button_width, button_height, TRUE);
      MoveWindow(cancel, client.right - margin - button_width, buttons_top, button_width, button_height, TRUE);
      MoveWindow(ok,
          client.right - margin - 2 * button_width - Scale(8),
          buttons_top,
          button_width,
          button_height,
          TRUE);
   }

   void TickEvery(const bool ticked) const
   {
      const auto count = ListView_GetItemCount(list);
      for (int index = 0; index < count; ++index)
      {
         ListView_SetCheckState(list, index, ticked ? TRUE : FALSE);
      }
   }

   // The launcher's window takes clicks again before this one goes, so Windows hands focus back to
   // it rather than to whatever else is open.
   void Close(const bool accept)
   {
      if (accept)
      {
         const auto count = ListView_GetItemCount(list);
         for (int index = 0; index < count; ++index)
         {
            if (ListView_GetCheckState(list, index) != 0)
            {
               chosen.push_back((*realms)[static_cast<std::size_t>(index)].key);
            }
         }
      }
      accepted = accept;
      done = true;
      EnableWindow(owner, TRUE);
      DestroyWindow(window);
   }
};

LRESULT CALLBACK PickerProcedure(HWND window, const UINT message, const WPARAM wparam, const LPARAM lparam)
{
   if (message == WM_NCCREATE)
   {
      SetWindowLongPtrW(window,
          GWLP_USERDATA,
          reinterpret_cast<LONG_PTR>(reinterpret_cast<CREATESTRUCTW*>(lparam)->lpCreateParams));
   }
   auto* picker = reinterpret_cast<Picker*>(GetWindowLongPtrW(window, GWLP_USERDATA));
   if (picker == nullptr || picker->window == nullptr)
   {
      return DefWindowProcW(window, message, wparam, lparam);
   }
   switch (message)
   {
      case WM_SIZE:
         picker->Layout();
         return 0;
      case WM_GETMINMAXINFO:
         reinterpret_cast<MINMAXINFO*>(lparam)->ptMinTrackSize = {picker->Scale(520), picker->Scale(360)};
         return 0;
      case WM_ERASEBKGND:
      {
         RECT client{};
         GetClientRect(window, &client);
         FillRect(reinterpret_cast<HDC>(wparam), &client, picker->background);
         return 1;
      }
      case WM_CTLCOLORSTATIC:
      case WM_CTLCOLORBTN:
         SetBkColor(reinterpret_cast<HDC>(wparam), picker->palette.window);
         SetTextColor(reinterpret_cast<HDC>(wparam), picker->palette.text);
         return reinterpret_cast<LRESULT>(picker->background);
      case WM_COMMAND:
         switch (LOWORD(wparam))
         {
            case IDOK:
               picker->Close(true);
               break;
            case IDCANCEL:
               picker->Close(false);
               break;
            case kTickAllId:
               picker->TickEvery(true);
               break;
            case kClearId:
               picker->TickEvery(false);
               break;
            default:
               break;
         }
         return 0;
      case WM_CLOSE:
         picker->Close(false);
         return 0;
      default:
         return DefWindowProcW(window, message, wparam, lparam);
   }
}

// A list's column headings are drawn in black whatever their theme. The list, which owns them, is
// asked before each one is drawn, and answers with the colour handed to it here.
LRESULT CALLBACK ListProcedure(HWND list,
    const UINT message,
    const WPARAM wparam,
    const LPARAM lparam,
    const UINT_PTR id,
    const DWORD_PTR text_colour)
{
   if (message == WM_NCDESTROY)
   {
      RemoveWindowSubclass(list, &ListProcedure, id);
   }
   else if (message == WM_NOTIFY)
   {
      const auto* draw = reinterpret_cast<const NMCUSTOMDRAW*>(lparam);
      if (draw->hdr.code == NM_CUSTOMDRAW && draw->hdr.hwndFrom == ListView_GetHeader(list))
      {
         if (draw->dwDrawStage == CDDS_PREPAINT)
         {
            return CDRF_NOTIFYITEMDRAW;
         }
         if (draw->dwDrawStage == CDDS_ITEMPREPAINT)
         {
            SetTextColor(draw->hdc, static_cast<COLORREF>(text_colour));
            return CDRF_DODEFAULT;
         }
      }
   }
   return DefSubclassProc(list, message, wparam, lparam);
}
}  // namespace

std::optional<std::vector<std::string>> launcher::PickRealms(HWND owner,
    const std::vector<RealmChoice>& realms,
    const std::vector<std::string>& picked,
    HFONT font,
    const bool dark)
{
   auto* instance = reinterpret_cast<HINSTANCE>(GetWindowLongPtrW(owner, GWLP_HINSTANCE));
   WNDCLASSEXW window_class{};
   window_class.cbSize = sizeof(window_class);
   window_class.lpfnWndProc = &PickerProcedure;
   window_class.hInstance = instance;
   window_class.hCursor = LoadCursorW(nullptr, IDC_ARROW);
   window_class.hbrBackground = GetSysColorBrush(COLOR_BTNFACE);
   window_class.lpszClassName = kPickerClass;
   RegisterClassExW(&window_class);  // fails harmlessly the second time the picker is opened

   Picker picker;
   picker.owner = owner;
   picker.dpi = GetDpiForWindow(owner);
   picker.palette = PaletteFor(dark);
   picker.background = CreateSolidBrush(picker.palette.window);
   picker.realms = &realms;

   RECT owner_area{};
   GetWindowRect(owner, &owner_area);
   const int width = picker.Scale(640);
   const int height = picker.Scale(520);
   HWND window = CreateWindowExW(WS_EX_DLGMODALFRAME | WS_EX_CONTROLPARENT,
       kPickerClass,
       L"Whose vassals become countries?",
       WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME,
       owner_area.left + (owner_area.right - owner_area.left - width) / 2,
       owner_area.top + (owner_area.bottom - owner_area.top - height) / 2,
       width,
       height,
       owner,
       nullptr,
       instance,
       &picker);
   if (window == nullptr)
   {
      DeleteObject(picker.background);
      return std::nullopt;
   }
   UseDarkTitleBar(window, dark);

   const auto add =
       [&](const wchar_t* type, const wchar_t* text, const DWORD style, const int id, const DWORD extended) {
          HWND control = CreateWindowExW(extended,
              type,
              text,
              WS_CHILD | WS_VISIBLE | style,
              0,
              0,
              0,
              0,
              window,
              reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)),
              instance,
              nullptr);
          SendMessageW(control, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
          ThemeControl(control, dark);
          return control;
       };
   picker.hint = add(L"STATIC",
       L"Tick the realms whose vassals should become countries of their own. With none ticked, your choice "
       L"applies to every realm.",
       SS_LEFT,
       0,
       0);
   picker.list =
       add(WC_LISTVIEWW, L"", WS_TABSTOP | LVS_REPORT | LVS_SINGLESEL | LVS_SHOWSELALWAYS, 0, WS_EX_CLIENTEDGE);
   picker.tick_all = add(L"BUTTON", L"Tick all", BS_PUSHBUTTON | WS_TABSTOP, kTickAllId, 0);
   picker.clear = add(L"BUTTON", L"Clear", BS_PUSHBUTTON | WS_TABSTOP, kClearId, 0);
   picker.ok = add(L"BUTTON", L"OK", BS_DEFPUSHBUTTON | WS_TABSTOP, IDOK, 0);
   picker.cancel = add(L"BUTTON", L"Cancel", BS_PUSHBUTTON | WS_TABSTOP, IDCANCEL, 0);

   ListView_SetExtendedListViewStyle(picker.list, LVS_EX_CHECKBOXES | LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);
   ListView_SetBkColor(picker.list, picker.palette.field);
   ListView_SetTextBkColor(picker.list, picker.palette.field);
   ListView_SetTextColor(picker.list, picker.palette.text);
   if (dark)
   {
      ThemeControl(ListView_GetHeader(picker.list), dark);
      SetWindowSubclass(picker.list, &ListProcedure, 1, picker.palette.text);
   }
   const auto add_column = [&picker](const int index, const wchar_t* title, const int width_in_pixels) {
      std::wstring text = title;
      LVCOLUMNW column{};
      column.mask = LVCF_TEXT | LVCF_WIDTH;
      column.pszText = text.data();
      column.cx = picker.Scale(width_in_pixels);
      ListView_InsertColumn(picker.list, index, &column);
   };
   add_column(0, L"Realm", 220);
   add_column(1, L"Rank", 80);
   add_column(2, L"Counties", 70);
   add_column(3, L"Vassals", 200);

   const std::set<std::string> already_picked(picked.begin(), picked.end());
   for (std::size_t index = 0; index < realms.size(); ++index)
   {
      const auto& realm = realms[index];
      auto name = Widen(realm.name);
      LVITEMW item{};
      item.mask = LVIF_TEXT;
      item.iItem = static_cast<int>(index);
      item.pszText = name.data();
      ListView_InsertItem(picker.list, &item);
      const auto set_text = [&picker, index](const int column, std::wstring text) {
         ListView_SetItemText(picker.list, static_cast<int>(index), column, text.data());
      };
      set_text(1, Widen(realm.rank));
      set_text(2, std::to_wstring(realm.counties));
      set_text(3, DescribeVassals(realm));
      ListView_SetCheckState(picker.list, static_cast<int>(index), already_picked.contains(realm.key) ? TRUE : FALSE);
   }

   // Only now does the window procedure start laying out and answering commands.
   picker.window = window;
   picker.Layout();
   EnableWindow(owner, FALSE);
   ShowWindow(window, SW_SHOW);
   SetFocus(picker.list);

   MSG message{};
   while (!picker.done)
   {
      const auto got = GetMessageW(&message, nullptr, 0, 0);
      if (got <= 0)
      {
         // The application is quitting: pass that on to the launcher's own loop.
         if (got == 0)
         {
            PostQuitMessage(static_cast<int>(message.wParam));
         }
         break;
      }
      if (IsDialogMessageW(window, &message) == FALSE)
      {
         TranslateMessage(&message);
         DispatchMessageW(&message);
      }
   }
   DeleteObject(picker.background);
   SetForegroundWindow(owner);
   return picker.accepted ? std::optional(picker.chosen) : std::nullopt;
}
