#pragma once

#include <cstdint>
#include <vector>

using i64 = int64_t;
using f64 = double;
using StrId = uint32_t;  // Dictionary-encoded string ID
using Date32 = int32_t;  // YYYYMMDD format

template <typename T>
class ColumnVector {
public:
    std::vector<T> data;
    // TODO: Add methods for vectorized operations
};