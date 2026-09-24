#include "vassal_contracts.hpp"

#include <sstream>
#include <string>

#include "CommonRegexes.h"
#include "Parser.h"
#include "ParserHelpers.h"

ck3::VassalContracts::VassalContracts(std::istream& input_stream)
{
   commonItems::parser parser;
   parser.registerKeyword("database", [this](std::istream& database_stream) {
      ParseDatabase(database_stream);
   });
   parser.registerRegex(commonItems::catchallRegex, commonItems::ignoreItem);
   parser.parseStream(input_stream);
}

void ck3::VassalContracts::ParseDatabase(std::istream& input_stream)
{
   commonItems::parser database_parser;
   database_parser.registerRegex(R"(\d+)", [this](const std::string& /*contract_id*/, std::istream& contract_stream) {
      // Ended contracts are left in the database as "id=none".
      const auto contract_blob = commonItems::stringOfItem(contract_stream).getString();
      if (!contract_blob.contains('{'))
      {
         return;
      }

      VassalContract contract;
      commonItems::parser contract_parser;
      contract_parser.registerKeyword("vassal", [&contract](std::istream& value_stream) {
         contract.vassal_id = commonItems::getLlong(value_stream);
      });
      contract_parser.registerKeyword("liege", [&contract](std::istream& value_stream) {
         contract.liege_id = commonItems::getLlong(value_stream);
      });
      contract_parser.registerKeyword("contract_group", [&contract](std::istream& value_stream) {
         contract.group = commonItems::getString(value_stream);
      });
      contract_parser.registerRegex(commonItems::catchallRegex, commonItems::ignoreItem);
      auto blob_stream = std::stringstream(contract_blob);
      contract_parser.parseStream(blob_stream);

      if (contract.vassal_id != 0 && contract.liege_id != 0)
      {
         contracts_.push_back(contract);
      }
   });
   database_parser.registerRegex(commonItems::catchallRegex, commonItems::ignoreItem);
   database_parser.parseStream(input_stream);
}

std::vector<ck3::VassalContract> ck3::VassalContracts::GetTributaries() const
{
   std::vector<VassalContract> tributaries;
   for (const auto& contract: contracts_)
   {
      if (contract.IsTributary())
      {
         tributaries.push_back(contract);
      }
   }
   return tributaries;
}
