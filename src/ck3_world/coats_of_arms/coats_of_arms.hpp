#ifndef CK3_COATS_OF_ARMS_H
#define CK3_COATS_OF_ARMS_H

#include <istream>
#include <map>
#include <set>
#include <string>

namespace ck3
{

// A coat of arms exactly as the save defines it. CK3 and EU5 share the Jomini format - pattern,
// colours, colored_emblem and textured_emblem with their instances - so the definition carries over
// verbatim; what may not is the art it names, which is listed so it can be supplied.
struct CoatOfArms
{
   std::string definition;         // the whole { ... } block
   std::set<std::string> textures;  // every pattern and emblem file it uses
};

// The save's coat_of_arms section. Its database holds every coat of arms in the game fully resolved,
// with no references to CK3's scripted templates.
class CoatsOfArms
{
  public:
   CoatsOfArms() = default;
   explicit CoatsOfArms(std::istream& input_stream);

   [[nodiscard]] const auto& GetCoatsOfArms() const { return coats_of_arms_; }

  private:
   void ParseDatabase(std::istream& input_stream);

   std::map<long long, CoatOfArms> coats_of_arms_;
};

}  // namespace ck3

#endif  // CK3_COATS_OF_ARMS_H
