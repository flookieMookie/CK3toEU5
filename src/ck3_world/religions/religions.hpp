#ifndef CK3_RELIGIONS_H
#define CK3_RELIGIONS_H

#include <memory>

#include "Parser.h"
#include "faith.hpp"
#include "religion.hpp"

namespace ck3
{

class Titles;
class Religions: commonItems::parser  // NOLINT : issues with error handling in parser
{
  public:
   Religions() = default;
   explicit Religions(std::istream& input_stream);

   [[nodiscard]] const auto& GetReligions() const { return religions_; }
   [[nodiscard]] const auto& GetFaiths() const { return faiths_; }
   // CK3 1.20's rites, by the ids counties and characters name them by.
   [[nodiscard]] const auto& GetRites() const { return rites_; }

   // The save's religion block: its religions, and before CK3 1.20 its faiths too.
   void ParseReligions(std::istream& input_stream);
   // CK3 1.20's own faiths block, faiths={ database={ ... } }.
   void ParseFaithDatabase(std::istream& input_stream);
   // CK3 1.20's rites block, rites={ database={ ... } }.
   void ParseRites(std::istream& input_stream);

   void LinkTitles(const Titles& titles);
   void LinkReligions();

  private:
   void ParseFaiths(std::istream& input_stream);
   void ParseOnlyReligions(std::istream& input_stream);

   std::map<long long, std::shared_ptr<Religion>> religions_;
   std::map<long long, std::shared_ptr<Faith>> faiths_;
   std::map<long long, std::shared_ptr<Faith>> rites_;
};
}  // namespace ck3

#endif  // CK3_RELIGIONS_H
