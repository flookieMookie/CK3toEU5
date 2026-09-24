#include "ck3_mods.hpp"

#include <exception>
#include <system_error>

#include "Log.h"
#include "ModLoader/ModLoader.h"

std::vector<Mod> ck3::ResolveMods(const std::filesystem::path& ck3_documents_directory,
    const std::vector<std::string>& mod_descriptors)
{
   if (mod_descriptors.empty())
   {
      return {};
   }
   Mods incoming;
   for (const auto& descriptor: mod_descriptors)
   {
      incoming.emplace_back("", descriptor);
   }

   commonItems::ModLoader loader;
   try
   {
      loader.loadMods(ck3_documents_directory, incoming);
      loader.sortMods();
   }
   catch (const std::exception& error)
   {
      // Most often the documents folder has no mod folder at all: the mods were never installed here.
      Log(LogLevel::Warning) << "Could not load the save's mods: " << error.what();
      return {};
   }
   return loader.getMods();
}

bool ck3::ChangesMap(const Mod& mod)
{
   std::error_code error;
   return std::filesystem::is_regular_file(mod.path / "map_data" / "definition.csv", error) ||
          std::filesystem::is_regular_file(mod.path / "map_data" / "provinces.png", error);
}

commonItems::ModFilesystem ck3::CK3Files(const std::filesystem::path& ck3_directory, const std::vector<Mod>& mods)
{
   return commonItems::ModFilesystem(ck3_directory / "game", mods);
}
