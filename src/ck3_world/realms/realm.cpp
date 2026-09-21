#include "realm.hpp"

#include <string>
#include <utility>

#include "src/ck3_world/characters/character.hpp"
#include "src/ck3_world/cultures/culture.hpp"
#include "src/ck3_world/religions/faith.hpp"

namespace
{
const std::string kUnknownName = "unknown";
}  // namespace

ck3::Realm::Realm(std::shared_ptr<Title> primary_title, std::shared_ptr<Character> holder):
    primary_title_(std::move(primary_title)),
    holder_(std::move(holder))
{
}

ck3::Level ck3::Realm::GetTier() const
{
   if (!primary_title_)
   {
      return Level::kUnknown;
   }
   return primary_title_->GetLevel();
}

std::string ck3::Realm::GetRealmName() const
{
   if (!primary_title_)
   {
      return kUnknownName;
   }
   // A player renaming a title, or a dynamic title, is the better name when present.
   if (primary_title_->GetCustomName().has_value() && !primary_title_->GetCustomName()->empty())
   {
      return *primary_title_->GetCustomName();
   }
   if (!primary_title_->GetName().empty())
   {
      return primary_title_->GetName();
   }
   return primary_title_->GetKey();
}

std::string ck3::Realm::GetRulerName() const
{
   if (!holder_ || holder_->GetName().empty())
   {
      return kUnknownName;
   }
   return holder_->GetName();
}

std::string ck3::Realm::GetCultureName() const
{
   if (!holder_ || !holder_->GetCulture().has_value())
   {
      return kUnknownName;
   }
   const auto culture = holder_->GetCulture()->GetPointer().lock();
   if (!culture || culture->GetName().empty())
   {
      return kUnknownName;
   }
   return culture->GetName();
}

std::string ck3::Realm::GetFaithName() const
{
   if (!holder_ || !holder_->GetFaith().has_value())
   {
      return kUnknownName;
   }
   const auto faith = holder_->GetFaith()->GetPointer().lock();
   if (!faith)
   {
      return kUnknownName;
   }
   if (!faith->GetCustomName().empty())
   {
      return faith->GetCustomName();
   }
   return faith->GetTag().empty() ? kUnknownName : faith->GetTag();
}

std::string ck3::Realm::GetGovernment() const
{
   if (!holder_ || !holder_->GetCharacterRealm().has_value())
   {
      return kUnknownName;
   }
   const auto& government = holder_->GetCharacterRealm()->GetGovernmentType();
   return government.empty() ? kUnknownName : government;
}
