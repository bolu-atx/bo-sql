#pragma once

#include <string>
#include <vector>
#include <variant>
#include <memory>
#include <stdexcept>
#include "../include/types.h"
#include "dictionary.h"

// Removed variant, using unique_ptr<Column> instead

struct TableColumn {
    std::string name;
    std::unique_ptr<Column> data;
};

struct Table {
    std::string name;
    std::vector<TableColumn> columns;
    std::shared_ptr<Dictionary> dict;

    // Helper to get column index by name
    size_t get_column_index(const std::string& col_name) const {
        for (size_t i = 0; i < columns.size(); ++i) {
            if (columns[i].name == col_name) return i;
        }
        throw std::runtime_error("Column not found: " + col_name);
    }

    // Get column data
    const std::unique_ptr<Column>& get_column_data(const std::string& col_name) const {
        return columns[get_column_index(col_name)].data;
    }
};