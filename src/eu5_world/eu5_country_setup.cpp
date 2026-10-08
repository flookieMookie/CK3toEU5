#include "eu5_country_setup.hpp"

#include <fstream>
#include <functional>
#include <regex>
#include <set>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include "Log.h"

namespace
{
const std::string kByteOrderMark = "\xEF\xBB\xBF";

// In text mode, so a file rewritten from one of these comes out with the same line endings.
std::string ReadFile(const std::filesystem::path& path)
{
   std::ifstream file(path);
   return {std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
}

std::string WithoutComments(const std::string& text)
{
   static const std::regex kComment("#[^\n]*");
   return std::regex_replace(text, kComment, "");
}

int BraceBalance(const std::string& line)
{
   int balance = 0;
   for (const char character: line.substr(0, line.find('#')))
   {
      balance += character == '{' ? 1 : (character == '}' ? -1 : 0);
   }
   return balance;
}

// Calls back with the key of every block opening at the top level of the text and each line inside
// it one brace in, the depth the setup files keep a block's own fields at.
void ForEachTopLevelField(const std::string& text,
    const std::function<void(const std::string& key, const std::string& line)>& field)
{
   static const std::regex kBlockStart(R"(^\s*([A-Za-z0-9_]+)\s*=\s*\{)");
   std::istringstream lines(text);
   std::string line;
   std::string key;
   int depth = 0;
   while (std::getline(lines, line))
   {
      if (line.starts_with(kByteOrderMark))
      {
         line.erase(0, kByteOrderMark.size());
      }
      if (depth == 0)
      {
         std::smatch match;
         if (std::regex_search(line, match, kBlockStart))
         {
            key = match[1].str();
         }
      }
      else if (depth == 1)
      {
         field(key, line);
      }
      depth += BraceBalance(line);
   }
}
}  // namespace

eu5::CountrySetup::CountrySetup(const std::filesystem::path& eu5_directory)
{
   const auto game = eu5_directory / "game";
   if (const auto folder = game / "main_menu" / "setup" / "templates"; std::filesystem::exists(folder))
   {
      for (const auto& entry: std::filesystem::directory_iterator(folder))
      {
         if (entry.path().extension() == ".txt")
         {
            AddTemplate(entry.path().stem().string(), ReadFile(entry.path()));
         }
      }
   }
   if (const auto folder = game / "in_game" / "setup" / "countries"; std::filesystem::exists(folder))
   {
      for (const auto& entry: std::filesystem::directory_iterator(folder))
      {
         if (entry.path().extension() == ".txt")
         {
            AddDefinitionFile(entry.path().filename().string(), ReadFile(entry.path()));
         }
      }
   }
   if (const auto folder = game / "in_game" / "common" / "religions"; std::filesystem::exists(folder))
   {
      for (const auto& entry: std::filesystem::directory_iterator(folder))
      {
         if (entry.path().extension() == ".txt")
         {
            AddReligions(ReadFile(entry.path()));
         }
      }
   }
   Log(LogLevel::Info) << "<> Read " << templates_.size() << " EU5 country templates, " << religion_of_tag_.size()
                       << " country definitions and " << group_of_religion_.size() << " religions.";
}

void eu5::CountrySetup::AddDefinitionFile(const std::string& name, const std::string& text)
{
   static const std::regex kField(R"(^\s*(culture_definition|religion_definition)\s*=\s*([A-Za-z0-9_]+))");
   ForEachTopLevelField(text, [this](const std::string& tag, const std::string& line) {
      std::smatch match;
      if (std::regex_search(line, match, kField))
      {
         (match[1].str() == "culture_definition" ? culture_of_tag_ : religion_of_tag_)[tag] = match[2].str();
      }
   });
   definition_files_.push_back({name, text});
}

void eu5::CountrySetup::AddReligions(const std::string& text)
{
   static const std::regex kGroup(R"(^\s*group\s*=\s*([A-Za-z0-9_]+))");
   ForEachTopLevelField(text, [this](const std::string& religion, const std::string& line) {
      std::smatch match;
      if (std::regex_search(line, match, kGroup))
      {
         group_of_religion_[religion] = match[1].str();
      }
   });
}

std::optional<std::string> eu5::CountrySetup::GovernmentTypeOf(const std::string& block) const
{
   static const std::regex kType(R"(\btype\s*=\s*(monarchy|republic|theocracy|tribe|steppe_horde)\b)");
   static const std::regex kInclude(R"re(\binclude\s*=\s*"?([A-Za-z0-9_]+)"?)re");
   const auto code = WithoutComments(block);
   if (std::smatch match; std::regex_search(code, match, kType))
   {
      return match[1].str();
   }
   for (auto include = std::sregex_iterator(code.begin(), code.end(), kInclude); include != std::sregex_iterator();
       ++include)
   {
      if (const auto found = templates_.find((*include)[1].str()); found != templates_.end())
      {
         if (const auto type = GovernmentTypeOf(found->second))
         {
            return type;
         }
      }
   }
   return std::nullopt;
}

std::vector<std::string> eu5::CountrySetup::GovernmentTemplatesOf(const std::string& block) const
{
   static const std::regex kInclude(R"re(\binclude\s*=\s*"?([A-Za-z0-9_]+)"?)re");
   const auto code = WithoutComments(block);
   std::vector<std::string> names;
   for (auto include = std::sregex_iterator(code.begin(), code.end(), kInclude); include != std::sregex_iterator();
       ++include)
   {
      const auto name = (*include)[1].str();
      if (!name.starts_with("expl_") && templates_.contains(name))
      {
         names.push_back(name);
      }
   }
   return names;
}

std::string eu5::CountrySetup::LandlockedVariantOf(const std::string& template_name) const
{
   if (template_name.ends_with("_no_coast"))
   {
      return template_name;
   }
   const auto landlocked = template_name + "_no_coast";
   return templates_.contains(landlocked) ? landlocked : template_name;
}

std::string eu5::CountrySetup::NotPresentVariantOf(const std::string& template_name) const
{
   if (template_name.ends_with("_not_present"))
   {
      return template_name;
   }
   if (const auto variant = template_name + "_not_present"; templates_.contains(variant))
   {
      return variant;
   }
   // The landlocked templates share their not-present version with the coastal ones.
   constexpr std::string_view kLandlocked = "_no_coast";
   if (template_name.ends_with(kLandlocked))
   {
      const auto variant = template_name.substr(0, template_name.size() - kLandlocked.size()) + "_not_present";
      if (templates_.contains(variant))
      {
         return variant;
      }
   }
   return template_name;
}

std::optional<std::string> eu5::CountrySetup::CultureOf(const std::string& tag) const
{
   const auto culture = culture_of_tag_.find(tag);
   return culture == culture_of_tag_.end() ? std::nullopt : std::optional(culture->second);
}

std::optional<std::string> eu5::CountrySetup::ReligionOf(const std::string& tag) const
{
   const auto religion = religion_of_tag_.find(tag);
   return religion == religion_of_tag_.end() ? std::nullopt : std::optional(religion->second);
}

std::optional<std::string> eu5::CountrySetup::GroupOf(const std::string& religion) const
{
   const auto group = group_of_religion_.find(religion);
   return group == group_of_religion_.end() ? std::nullopt : std::optional(group->second);
}

std::string eu5::Redefine(const std::string& definitions, const std::map<std::string, Redefinition>& redefinitions)
{
   static const std::regex kBlockStart(R"(^\s*([A-Za-z0-9_]+)\s*=\s*\{)");
   static const std::regex kCulture(R"((culture_definition\s*=\s*)[A-Za-z0-9_]+)");
   static const std::regex kReligion(R"((religion_definition\s*=\s*)[A-Za-z0-9_]+)");
   static const std::regex kHistoric(R"(^\s*is_historic\s*=)");
   std::istringstream lines(definitions);
   std::vector<std::string> output;
   std::string line;
   const Redefinition* redefinition = nullptr;
   int depth = 0;
   while (std::getline(lines, line))
   {
      const auto opening = depth == 0;
      depth += BraceBalance(line);
      if (opening)
      {
         redefinition = nullptr;
         const auto key_line = line.starts_with(kByteOrderMark) ? line.substr(kByteOrderMark.size()) : line;
         if (std::smatch match; std::regex_search(key_line, match, kBlockStart))
         {
            if (const auto found = redefinitions.find(match[1].str()); found != redefinitions.end())
            {
               redefinition = &found->second;
            }
         }
         output.push_back(line);
         if (redefinition != nullptr && redefinition->historic == true && depth == 1)
         {
            output.emplace_back("\tis_historic = yes");
         }
         continue;
      }
      if (redefinition != nullptr && depth == 1)
      {
         // Whatever the tag said about being historic, the redefinition decides it.
         if (redefinition->historic.has_value() && std::regex_search(line, kHistoric))
         {
            continue;
         }
         if (redefinition->culture.has_value())
         {
            line = std::regex_replace(line, kCulture, "$1" + *redefinition->culture);
         }
         if (redefinition->religion.has_value())
         {
            line = std::regex_replace(line, kReligion, "$1" + *redefinition->religion);
         }
      }
      output.push_back(line);
   }
   std::string text;
   for (const auto& output_line: output)
   {
      text += output_line + "\n";
   }
   if (!definitions.ends_with('\n') && !text.empty())
   {
      text.pop_back();
   }
   return text;
}
