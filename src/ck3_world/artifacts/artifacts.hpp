#ifndef CK3_ARTIFACTS_H
#define CK3_ARTIFACTS_H

#include <Date.h>

#include <istream>
#include <optional>
#include <string>
#include <vector>

namespace ck3
{

// A CK3 artifact: a crown, a sword, a holy book, a tapestry for the court.
struct Artifact
{
   long long id = 0;
   std::string name;  // as the save renders it - "Papal Tiara"
   std::string description;
   std::string type;    // sword, regalia, book, tapestry...
   std::string rarity;  // common, masterwork, famed, illustrious
   long long owner = 0;
   // When its history begins: when it was made, or for those older than the campaign, the start.
   std::optional<date> created;
};

// The save's artifacts section.
class Artifacts
{
  public:
   Artifacts() = default;
   explicit Artifacts(std::istream& input_stream);

   [[nodiscard]] const auto& GetArtifacts() const { return artifacts_; }

  private:
   std::vector<Artifact> artifacts_;
};

}  // namespace ck3

#endif  // CK3_ARTIFACTS_H
