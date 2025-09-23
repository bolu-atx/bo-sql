#pragma once

#include <string>
#include <vector>
#include <variant>
#include <memory>
#include <stdexcept>
#include "../include/types.h"
#include "dictionary.h"

using ColumnData = std::variant<ColumnVector<i64>, ColumnVector<f64>, ColumnVector<StrId>, ColumnVector<Date32> >;

struct Column {
    std::string name;
    ColumnData data;
};

struct Table {
    std::string name;
    std::vector<Column> columns;
    std::shared_ptr<Dictionary> dict;

    // Helper to get column index by name
    size_t get_column_index(const std::string& col_name) const {
        for (size_t i = 0; i < columns.size(); ++i) {
            if (columns[i].name == col_name) return i;
        }
        throw std::runtime_error("Column not found: " + col_name);
    }

    // Get column data
    const ColumnData& get_column_data(const std::string& col_name) const {
        return columns[get_column_index(col_name)].data;
    }
};