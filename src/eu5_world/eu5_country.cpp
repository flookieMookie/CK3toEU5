#include "eu5_country.hpp"

#include <utility>

eu5::Country::Country(std::string tag, std::shared_ptr<ck3::Realm> source_realm):
    tag_(std::move(tag)),
    source_realm_(std::move(source_realm))
{
}
