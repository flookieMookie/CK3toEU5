#ifndef OUT_VANILLA_START_FILE_H
#define OUT_VANILLA_START_FILE_H

#include <filesystem>
#include <functional>
#include <optional>
#include <set>
#include <string>

#include "src/eu5_world/eu5_vanilla_countries.hpp"
#include "src/eu5_world/eu5_world.hpp"
#include "src/output/out_file_classes/output_file.hpp"

namespace out
{

// Walks the entries of one of EU5's own start files - anything beginning entry_depth braces in - and
// replaces each with what transform returns for it, dropping it when that is empty. Everything
// outside the entries is kept as it is.
[[nodiscard]] std::string TransformEntries(const std::string& contents,
    int entry_depth,
    const std::function<std::optional<std::string>(const std::string& entry)>& transform);

// Keeps the entries of one of EU5's own start files that concern only the given countries.
//
// The file is copied line by line; an entry is anything that begins at entry_depth braces in, and
// is kept only if every country tag it names - a three letter uppercase token - is allowed.
// Comments are ignored when looking for tags. Everything outside the entries is kept as it is.
[[nodiscard]] std::string KeepEntriesAbout(const std::string& contents,
    int entry_depth,
    const std::set<std::string>& allowed_tags);

// Every country tag - three letter uppercase token - named outside comments.
[[nodiscard]] std::set<std::string> TagsNamedIn(const std::string& text);

// Writes one of EU5's start files - wars, rivals, opinions, armies, AI personalities - keeping only
// what concerns the vanilla countries the conversion keeps on land CK3 doesn't cover.
//
// Left as they are, these carry EU5's 1337 into the converted world: a converted country that
// happens to share a tag with a vanilla one would start in a vanilla war, hate a vanilla rival, and
// march armies led by characters who no longer exist. The converted countries' own relations come
// from CK3 instead.
class VanillaStartFile: public OutputFile
{
  public:
   VanillaStartFile(const std::string& name,
       FileWriter& file_writer,
       const eu5::EU5World& eu5_world,
       const eu5::VanillaCountries& vanilla_countries,
       std::filesystem::path eu5_directory,
       int entry_depth);

   void Create(const std::filesystem::path& folder_path) override;

  private:
   const eu5::EU5World& eu5_world_;
   const eu5::VanillaCountries& vanilla_countries_;
   std::filesystem::path eu5_directory_;
   int entry_depth_;
};

}  // namespace out

#endif  // OUT_VANILLA_START_FILE_H
