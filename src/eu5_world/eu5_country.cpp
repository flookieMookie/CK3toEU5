#include "eu5_country.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <utility>

#include "src/ck3_world/realms/realm.hpp"

eu5::Country::Country(std::string tag, std::shared_ptr<ck3::Realm> source_realm):
    tag_(std::move(tag)),
    source_realm_(std::move(source_realm))
{
}

namespace
{
// EU5 keys are lowercase identifiers, so anything from CK3 has to be reduced to that.
std::string ToKey(const std::string& text)
{
   std::string key;
   for (const char character: text)
   {
      if (std::isalnum(static_cast<unsigned char>(character)) != 0)
      {
         key += static_cast<char>(std::tolower(static_cast<unsigned char>(character)));
      }
      else if (!key.empty() && key.back() != '_')
      {
         key += '_';
      }
   }
   while (!key.empty() && key.back() == '_')
   {
      key.pop_back();
   }
   return key;
}
}  // namespace

bool eu5::Country::HasRuler() const
{
   if (!source_realm_ || !culture_.has_value() || !religion_.has_value())
   {
      return false;
   }
   return source_realm_->GetHolder() != nullptr && !GetRulerName().empty();
}

std::string eu5::Country::GetRulerId() const
{
   return "ck3_" + ToKey(tag_) + "_ruler";
}

std::string eu5::Country::GetRulerName() const
{
   if (!source_realm_)
   {
      return {};
   }
   const auto name = source_realm_->GetRulerName();
   return name == "unknown" ? std::string{} : CleanCK3Name(name);
}

std::string eu5::Country::GetRulerNameKey() const
{
   return CharacterNameKey(GetRulerName());
}

std::string eu5::CharacterNameKey(const std::string& clean_name)
{
   const auto key = ToKey(clean_name);
   return key.empty() ? std::string{} : "ck3_name_" + key;
}

namespace
{
// Minimal UTF-8 encoder; CK3 escapes never exceed the basic multilingual plane.
void AppendUtf8(std::string& target, unsigned int codepoint)
{
   if (codepoint < 0x80U)
   {
      target += static_cast<char>(codepoint);
   }
   else if (codepoint < 0x800U)
   {
      target += static_cast<char>(0xC0U | (codepoint >> 6U));
      target += static_cast<char>(0x80U | (codepoint & 0x3FU));
   }
   else
   {
      target += static_cast<char>(0xE0U | (codepoint >> 12U));
      target += static_cast<char>(0x80U | ((codepoint >> 6U) & 0x3FU));
      target += static_cast<char>(0x80U | (codepoint & 0x3FU));
   }
}

// The melt writes its escapes in uppercase hex - Zhuo_68B2 - so a name that merely has four hex
// letters after an underscore, Domnall_Dabaill or Abu_Abdallah, is left as it is.
bool IsEscapeDigit(char character)
{
   return (character >= '0' && character <= '9') || (character >= 'A' && character <= 'F');
}

// Half of a UTF-16 surrogate pair is no character on its own, and written as UTF-8 it makes the
// whole file invalid.
bool IsSurrogate(unsigned int codepoint)
{
   return codepoint >= 0xD800U && codepoint <= 0xDFFFU;
}
}  // namespace

std::string eu5::CleanCK3Name(const std::string& name)
{
   std::string clean;
   for (std::size_t index = 0; index < name.size(); ++index)
   {
      if (name[index] != '_')
      {
         clean += name[index];
         continue;
      }
      // _HHHH is a codepoint the melt escaped.
      if (index + 4 < name.size() && IsEscapeDigit(name[index + 1]) && IsEscapeDigit(name[index + 2]) &&
          IsEscapeDigit(name[index + 3]) && IsEscapeDigit(name[index + 4]))
      {
         if (const auto codepoint = static_cast<unsigned int>(std::stoul(name.substr(index + 1, 4), nullptr, 16));
             !IsSurrogate(codepoint))
         {
            AppendUtf8(clean, codepoint);
            index += 4;
            continue;
         }
      }
      // Between the end of one word and the capital of the next it is a space: Aillil_Fland_Becc.
      if (!clean.empty() && std::islower(static_cast<unsigned char>(clean.back())) != 0 && index + 1 < name.size() &&
          std::isupper(static_cast<unsigned char>(name[index + 1])) != 0)
      {
         clean += ' ';
         continue;
      }
      // A lost character. Drop the underscore, and lowercase the capital it stranded mid-word.
      if (clean.size() > 1 && std::isupper(static_cast<unsigned char>(clean.back())) != 0)
      {
         clean.back() = static_cast<char>(std::tolower(static_cast<unsigned char>(clean.back())));
      }
   }
   return clean;
}

int eu5::StartingGoldFor(const double ck3_gold)
{
   constexpr double kLowest = -500.0;
   constexpr double kHighest = 2500.0;
   return static_cast<int>(std::lround(std::clamp(ck3_gold, kLowest, kHighest)));
}

int eu5::AbilityFromSkill(const int ck3_skill)
{
   constexpr int kBase = 15;
   constexpr int kPerSkillPoint = 6;
   return std::clamp(kBase + kPerSkillPoint * ck3_skill, 0, 100);
}
