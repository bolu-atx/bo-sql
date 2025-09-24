#include "storage/table.h"

size_t Table::get_column_index(const std::string& col_name) const {
    for (size_t i = 0; i < columns.size(); ++i) {
        if (columns[i].name == col_name) return i;
    }
    throw std::runtime_error("Column not found: " + col_name);
}

const std::unique_ptr<Column>& Table::get_column_data(const std::string& col_name) const {
    return columns[get_column_index(col_name)].data;
}