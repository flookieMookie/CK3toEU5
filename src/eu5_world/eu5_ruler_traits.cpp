#include "eu5_ruler_traits.hpp"

#include <algorithm>
#include <functional>
#include <map>

namespace
{
struct TraitRule
{
   std::string ck3_trait;
   std::string eu5_trait;
   // Beyond the exclusions, what EU5's allow block asks of the character's abilities.
   std::function<bool(const eu5::Abilities&)> abilities_allow = [](const eu5::Abilities&) {
      return true;
   };
};

// In order of preference: CK3's personality traits describe the ruler best, then their lifestyle.
const std::vector<TraitRule> kRules = {
    {"brave", "bold_fighter"},
    {"craven",
        "craven",
        [](const eu5::Abilities& a) {
           return a.mil < 70;
        }},
    {"just",
        "just",
        [](const eu5::Abilities& a) {
           return a.adm > 33;
        }},
    {"callous", "cruel"},
    {"sadistic", "malevolent"},
    {"compassionate", "kind_hearted"},
    {"calm", "calm"},
    {"patient", "careful"},
    {"deceitful", "secretive"},
    {"honest",
        "incorruptible",
        [](const eu5::Abilities& a) {
           return a.adm >= 50;
        }},
    {"greedy", "greedy"},
    {"zealous", "zealot"},
    {"cynical", "free_thinker"},
    {"gregarious",
        "charismatic_negotiator",
        [](const eu5::Abilities& a) {
           return a.dip >= 50;
        }},
    {"trusting",
        "naive",
        [](const eu5::Abilities& a) {
           return a.dip < 80;
        }},
    {"stubborn",
        "strict",
        [](const eu5::Abilities& a) {
           return a.mil >= 50;
        }},
    {"diligent", "industrious"},
    {"strategist",
        "tactical_genius",
        [](const eu5::Abilities& a) {
           return a.mil >= 50;
        }},
    {"architect", "architectural_visionary"},
    {"scholar", "scholar"},
    {"administrator",
        "lawgiver",
        [](const eu5::Abilities& a) {
           return a.adm >= 66;
        }},
    {"diplomat",
        "silver_tongue",
        [](const eu5::Abilities& a) {
           return a.dip >= 50;
        }},
    {"schemer",
        "intricate_web_weaver",
        [](const eu5::Abilities& a) {
           return a.adm >= 50;
        }},
    {"avaricious",
        "midas_touched",
        [](const eu5::Abilities& a) {
           return a.adm >= 50;
        }},
    {"aggressive_attacker",
        "conqueror",
        [](const eu5::Abilities& a) {
           return a.mil >= 50;
        }},
    {"drunkard", "drunkard"},
    {"stuttering",
        "babbling_buffoon",
        [](const eu5::Abilities& a) {
           return a.dip < 80;
        }},
};

// The traits each EU5 trait's allow block rules out.
const std::map<std::string, std::set<std::string>> kExcludes = {
    {"just", {"cruel", "malevolent"}},
    {"cruel", {"just"}},
    {"kind_hearted", {"cruel", "malevolent"}},
    {"free_thinker", {"careful", "zealot", "tolerant"}},
    {"careful", {"naive", "free_thinker", "bold_fighter"}},
    {"secretive", {"loose_lips"}},
    {"intricate_web_weaver", {"loose_lips"}},
    {"zealot", {"tolerant", "free_thinker", "drunkard", "sinner"}},
    {"midas_touched", {"greedy"}},
    {"incorruptible", {"embezzler"}},
    {"charismatic_negotiator", {"babbling_buffoon"}},
    {"silver_tongue", {"babbling_buffoon", "naive"}},
    {"bold_fighter", {"careful", "craven"}},
    {"babbling_buffoon", {"charismatic_negotiator", "silver_tongue"}},
    {"drunkard", {"pious", "entrepreneur", "zealot"}},
    {"greedy", {"midas_touched"}},
    {"naive", {"silver_tongue", "careful"}},
    {"craven", {"inspiring_leader", "bold_fighter"}},
    {"malevolent", {"benevolent", "kind_hearted", "just"}},
};

bool Excludes(const std::string& trait, const std::string& other)
{
   const auto excluded = kExcludes.find(trait);
   return excluded != kExcludes.end() && excluded->second.contains(other);
}

constexpr std::size_t kMostTraits = 3;
}  // namespace

std::vector<std::string> eu5::RulerTraitsFor(const std::set<std::string>& ck3_traits, const Abilities& abilities)
{
   std::vector<std::string> traits;
   for (const auto& rule: kRules)
   {
      if (traits.size() == kMostTraits)
      {
         break;
      }
      if (!ck3_traits.contains(rule.ck3_trait) || !rule.abilities_allow(abilities) ||
          std::ranges::find(traits, rule.eu5_trait) != traits.end())
      {
         continue;
      }
      // EU5 checks each trait against the others, either way round.
      if (std::ranges::any_of(traits, [&rule](const std::string& trait) {
             return Excludes(rule.eu5_trait, trait) || Excludes(trait, rule.eu5_trait);
          }))
      {
         continue;
      }
      traits.push_back(rule.eu5_trait);
   }
   return traits;
}
