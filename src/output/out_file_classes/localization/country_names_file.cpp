#include "country_names_file.hpp"

#include <external/commonItems/Log.h>

#include <set>
#include <sstream>
#include <string>
#include <utility>

#include "src/ck3_world/characters/character.hpp"
#include "src/ck3_world/realms/realm.hpp"
#include "src/ck3_world/titles/title.hpp"
#include "src/eu5_world/eu5_country.hpp"

namespace
{
// Paradox localisation files are UTF-8 with a byte order mark; the game will not read them without.
const std::string kByteOrderMark = "\xEF\xBB\xBF";

// CK3 names arrive with control characters around dynamic name segments, and a stray quote would
// end the localisation string early.
std::string Sanitize(const std::string& name)
{
   std::string clean;
   for (const char character: name)
   {
      if (static_cast<unsigned char>(character) < 0x20)
      {
         continue;
      }
      if (character == '"')
      {
         clean += '\'';
         continue;
      }
      clean += character;
   }
   return clean;
}

void WriteEntry(std::ostringstream& output, const std::string& key, const std::string& text)
{
   output << " " << key << ": \"" << Sanitize(text) << "\"\n";
}

// EU5 shows a country's adjective wherever it describes something as belonging to the country - its
// army, its people. CK3 keeps one for every title; where it doesn't, the name reads better than a
// raw key.
std::string AdjectiveFor(const ck3::Realm& realm, const std::string& name)
{
   const auto& primary_title = realm.GetPrimaryTitle();
   if (primary_title && !primary_title->GetAdjective().empty())
   {
      return eu5::CleanCK3Name(primary_title->GetAdjective());
   }
   return name;
}
}  // namespace

namespace out
{

CountryNamesFile::CountryNamesFile(const std::string& name,
    FileWriter& file_writer,
    const eu5::EU5World& eu5_world,
    const commonItems::LocalizationDatabase& ck3_culture_names,
    std::string language):
    OutputFile(name, file_writer),
    eu5_world_(eu5_world),
    ck3_culture_names_(ck3_culture_names),
    language_(std::move(language))
{
}

void CountryNamesFile::Create(const std::filesystem::path& folder_path)
{
   Log(LogLevel::Info) << "\tCreating " << GetName();

   std::ostringstream output;
   output << kByteOrderMark << "l_" << language_ << ":\n";

   int written = 0;
   std::set<std::string> ruler_names;
   for (const auto& country: eu5_world_.GetCountries())
   {
      if (country->GetLocations().empty())
      {
         continue;
      }
      const auto name = eu5::CleanCK3Name(country->GetSourceRealm()->GetRealmName());
      if (Sanitize(name).empty())
      {
         continue;
      }
      WriteEntry(output, country->GetTag(), name);
      WriteEntry(output, country->GetTag() + "_ADJ", AdjectiveFor(*country->GetSourceRealm(), name));
      ++written;

      // Ruler names are written as keys so the game shows them properly rather than a raw token.
      if (country->HasRuler())
      {
         const auto key = country->GetRulerNameKey();
         if (!key.empty() && ruler_names.insert(key).second)
         {
            WriteEntry(output, key, country->GetRulerName());
         }
      }
      for (const auto& member: country->GetFamily())
      {
         const auto member_name = eu5::CleanCK3Name(member.character->GetName());
         const auto key = eu5::CharacterNameKey(member_name);
         if (!key.empty() && ruler_names.insert(key).second)
         {
            WriteEntry(output, key, member_name);
         }
      }
   }

   const auto cultures = eu5_world_.GetCultureResolver().GetUsedGeneratedCultures();
   for (const auto& [key, definition]: cultures)
   {
      WriteEntry(output, key, eu5::CultureDisplayName(key, definition, ck3_culture_names_, language_));
   }

   Log(LogLevel::Info) << "\t<> Wrote " << written << " country names and " << cultures.size() << " culture names in "
                       << language_ << ".";
   UseFileWriter().CreateEmptyAndWrite(folder_path / GetName(), output.str());
}

}  // namespace out
