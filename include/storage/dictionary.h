#pragma once

#include <string>
#include <vector>
#include <algorithm>
#include "types.h"

// Dictionary for encoding strings to IDs and vice versa
class Dictionary {
public:
    std::vector<std::string> strings;

    StrId get_or_add(const std::string& s);
    const std::string& get(StrId id) const;
};