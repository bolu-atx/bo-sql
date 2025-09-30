#pragma once

#include <memory>
#include <span>
#include <stdexcept>
#include <vector>
#include "types.h"

namespace bosql {

struct ColumnSlice {
    const void* data;
    TypeId type;
    size_t length;
    std::shared_ptr<void> owner;
};

struct ExecBatch {
    std::vector<ColumnSlice> columns;
    size_t length = 0;

    void clear() {
        columns.clear();
        length = 0;
    }
};

template<typename T>
std::span<const T> get_col(const ExecBatch& batch, size_t i) {
    if (batch.columns[i].type != type_id_for<T>()) {
        throw std::runtime_error("Type mismatch");
    }
    return {reinterpret_cast<const T*>(batch.columns[i].data), batch.columns[i].length};
}

}
