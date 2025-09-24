#pragma once

#include <string>
#include <vector>
#include <variant>
#include <memory>
#include <stdexcept>
#include "types.h"
#include "storage/dictionary.h"

// Removed variant, using unique_ptr<Column> instead

// Represents a column in a table with name and data
struct TableColumn {
    std::string name;
    std::unique_ptr<Column> data;
};

// Represents a table with columns and a shared dictionary for strings
struct Table {
    std::string name;
    std::vector<TableColumn> columns;
    std::shared_ptr<Dictionary> dict;

    // Helper to get column index by name
    size_t get_column_index(const std::string& col_name) const;

    // Get column data
    const std::unique_ptr<Column>& get_column_data(const std::string& col_name) const;
};