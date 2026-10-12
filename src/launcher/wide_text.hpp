#ifndef LAUNCHER_WIDE_TEXT_H
#define LAUNCHER_WIDE_TEXT_H

#include <windows.h>

#include <string>

namespace launcher
{

// The launcher's logic speaks UTF-8; Windows' own controls want UTF-16.
[[nodiscard]] inline std::wstring Widen(const std::string& text)
{
   if (text.empty())
   {
      return {};
   }
   const auto length = MultiByteToWideChar(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), nullptr, 0);
   std::wstring wide(static_cast<std::size_t>(length), L'\0');
   MultiByteToWideChar(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), wide.data(), length);
   return wide;
}

[[nodiscard]] inline std::string Narrow(const std::wstring& text)
{
   if (text.empty())
   {
      return {};
   }
   const auto length =
       WideCharToMultiByte(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), nullptr, 0, nullptr, nullptr);
   std::string narrow(static_cast<std::size_t>(length), '\0');
   WideCharToMultiByte(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), narrow.data(), length, nullptr, nullptr);
   return narrow;
}

}  // namespace launcher

#endif  // LAUNCHER_WIDE_TEXT_H
