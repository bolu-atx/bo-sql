#pragma once

#include <string>
#include <vector>
#include <variant>
#include <unordered_map>
#include <vector>
#include <string>
#include "types.h"

// Column statistics
struct ColumnStats {
    i64 min_i64 = 0, max_i64 = 0;
    f64 min_f64 = 0.0, max_f64 = 0.0;
    Date32 min_date = 0, max_date = 0;
    size_t ndv = 0;
};

// Column metadata with statistics
struct ColumnMeta {
    std::string name;
    TypeId type;
    ColumnStats stats;

    ColumnMeta(std::string n, TypeId t, size_t ndv = 0)
        : name(std::move(n)), type(t) { stats.ndv = ndv; }
};

// Table metadata
struct TableMeta {
    std::string name;
    std::vector<ColumnMeta> columns;
    size_t row_count;
    // Additional stats could go here if needed

    TableMeta() = default;
    TableMeta(std::string n, std::vector<ColumnMeta> cols, size_t rows)
        : name(std::move(n)), columns(std::move(cols)), row_count(rows) {}
};

// Catalog for managing table metadata
class Catalog {
private:
    std::unordered_map<std::string, TableMeta> tables_;

public:
    // Register a table in the catalog
    void register_table(TableMeta table_meta);

    // Get table metadata by name
    const TableMeta* get_table(const std::string& name) const;

    // List all table names
    std::vector<std::string> list_tables() const;
};