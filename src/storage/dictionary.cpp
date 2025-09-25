#include "storage/dictionary.h"

namespace bosql {

StrId Dictionary::get_or_add(const std::string& s) {
    auto it = std::find(strings.begin(), strings.end(), s);
    if (it != strings.end()) return static_cast<StrId>(it - strings.begin());
    strings.push_back(s);
    return static_cast<StrId>(strings.size() - 1);
}

const std::string& Dictionary::get(StrId id) const { return strings[id]; }

} // namespace bosql