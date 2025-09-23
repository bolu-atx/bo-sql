#pragma once

#include <string>
#include <vector>
#include <algorithm>
#include "../include/types.h"

class Dictionary {
public:
    std::vector<std::string> strings;

    StrId get_or_add(const std::string& s) {
        auto it = std::find(strings.begin(), strings.end(), s);
        if (it != strings.end()) return static_cast<StrId>(it - strings.begin());
        strings.push_back(s);
        return static_cast<StrId>(strings.size() - 1);
    }

    const std::string& get(StrId id) const { return strings[id]; }
};