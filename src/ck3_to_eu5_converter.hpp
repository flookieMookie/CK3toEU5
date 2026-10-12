#ifndef SRC_CK3_TO_EU5_CONVERTER_HPP
#define SRC_CK3_TO_EU5_CONVERTER_HPP



#include <external/commonItems/ConverterVersion.h>
#include <external/commonItems/GameVersion.h>

#include "configuration/configuration.hpp"


namespace ck3_to_eu5
{
class Converter
{
  public:
   Converter(configuration::Configuration configuration, commonItems::ConverterVersion converter_version);
   void Convert();
   // Reads the save and reports its realms, one a line, instead of converting it: what the launcher
   // shows the player to pick whose vassals to keep.
   void ListRealms();

  private:
   configuration::Configuration configuration_;
   commonItems::ConverterVersion converter_version_;
};

}  // namespace ck3_to_eu5



#endif  // SRC_CK3_TO_EU5_CONVERTER_HPP