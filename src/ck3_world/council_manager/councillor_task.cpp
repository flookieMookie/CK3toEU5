#include "councillor_task.hpp"

#include <iostream>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>

#include "CommonRegexes.h"
#include "Log.h"
#include "Parser.h"
#include "ParserHelpers.h"
#include "src/ck3_world/id_pointer_pair.hpp"


ck3::CouncillorTask::CouncillorTask(std::istream& input_stream, long long task_id): task_id_(task_id)
{
   commonItems::parser parser;
   parser.registerKeyword("type", [this](const std::string&, std::istream& input_stream) {
      type_ = commonItems::singleString(input_stream).getString();
   });
   parser.registerKeyword("owner", [this](const std::string&, std::istream& input_stream) {
      holder_ = IdPointerPair<Character>(commonItems::singleLlong(input_stream).getLlong());
   });
   parser.registerKeyword("court_owner", [this](const std::string&, std::istream& input_stream) {
      court_owner_ = IdPointerPair<Character>(commonItems::singleLlong(input_stream).getLlong());
   });
   parser.registerRegex(commonItems::catchallRegex, commonItems::ignoreItem);

   parser.parseStream(input_stream);
   parser.clearRegisteredKeywords();
}

void ck3::CouncillorTask::LinkCharacters(const std::map<long long, std::shared_ptr<Character>>& characters)
{
   if (!holder_.has_value())
   {
      // Frozen or broken task
      return;
   }
   // CK3 prunes characters over a long campaign. A task left pointing at one is treated like the
   // frozen tasks above rather than aborting the conversion.
   if (!characters.contains(holder_->GetID()))
   {
      Log(LogLevel::Debug) << "Councillor task " << task_id_ << " has holder " << holder_->GetID()
                           << " who is no longer in the save, ignoring the task.";
      holder_.reset();
      return;
   }
   holder_->SetPointer(characters.at(holder_->GetID()));
   if (characters.contains(court_owner_.GetID()))
   {
      court_owner_.SetPointer(characters.at(court_owner_.GetID()));
   }
   else
   {
      Log(LogLevel::Debug) << "Councillor task " << task_id_ << " has court owner " << court_owner_.GetID()
                           << " who is no longer in the save.";
   }
}