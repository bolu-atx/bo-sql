#pragma once

#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <algorithm>
#include <limits>
#include <utility>
#include <set>
#include "../include/types.h"
#include "table.h"
#include "../catalog/catalog.h"

std::pair<Table, TableMeta> load_csv(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open file: " + filename);
    }

    Table table;
    table.dict = std::make_shared<Dictionary>();
    std::vector<ColumnMeta> column_metas;

    std::string line;
    std::vector<std::string> headers;
    std::vector<std::vector<std::string> > rows;

    // Read headers
    if (std::getline(file, line)) {
        std::stringstream ss(line);
        std::string token;
        while (std::getline(ss, token, ',')) {
            headers.push_back(token);
        }
    }

    // Read data rows
    while (std::getline(file, line)) {
        std::stringstream ss(line);
        std::string token;
        std::vector<std::string> row;
        while (std::getline(ss, token, ',')) {
            row.push_back(token);
        }
        if (row.size() != headers.size()) {
            throw std::runtime_error("Row size mismatch");
        }
        rows.push_back(row);
    }

    // Infer types and create columns
    size_t num_rows = rows.size();
    for (size_t col = 0; col < headers.size(); ++col) {
        std::string col_name = headers[col];
        TableColumn column;
        column.name = col_name;
        ColumnMeta meta(col_name, TypeId::STRING); // default to string

        // Check if all values are date (8 digits)
        bool all_date = true;
        Date32 min_date = std::numeric_limits<Date32>::max();
        Date32 max_date = std::numeric_limits<Date32>::min();
        for (const auto& row : rows) {
            if (row[col].size() != 8) {
                all_date = false;
                break;
            }
            try {
                Date32 d = std::stoi(row[col]);
                if (d < 19000000 || d > 21000000) all_date = false;
                min_date = std::min(min_date, d);
                max_date = std::max(max_date, d);
            } catch (...) {
                all_date = false;
                break;
            }
        }
        if (all_date && !rows.empty()) {
            std::vector<Date32> data;
            data.reserve(num_rows);
            for (const auto& row : rows) {
                data.push_back(std::stoi(row[col]));
            }
            std::set<Date32> uniques(data.begin(), data.end());
            column.data.reset(new ColumnVector<Date32>(std::move(data)));
            meta.type = TypeId::DATE32;
            meta.stats.min_date = min_date;
            meta.stats.max_date = max_date;
            meta.stats.ndv = uniques.size();
            table.columns.push_back(std::move(column));
            column_metas.push_back(std::move(meta));
            continue;
        }

        // Check if all values are i64
        bool all_i64 = true;
        i64 min_i64 = std::numeric_limits<i64>::max();
        i64 max_i64 = std::numeric_limits<i64>::min();
        for (const auto& row : rows) {
            try {
                f64 val = std::stod(row[col]);
                if (val != std::floor(val) || val < std::numeric_limits<i64>::min() || val > std::numeric_limits<i64>::max()) {
                    all_i64 = false;
                    break;
                }
                i64 ival = static_cast<i64>(val);
                min_i64 = std::min(min_i64, ival);
                max_i64 = std::max(max_i64, ival);
            } catch (...) {
                all_i64 = false;
                break;
            }
        }
        if (all_i64 && !rows.empty()) {
            std::vector<i64> data;
            data.reserve(num_rows);
            for (const auto& row : rows) {
                data.push_back(static_cast<i64>(std::stod(row[col])));
            }
            std::set<i64> uniques(data.begin(), data.end());
            column.data.reset(new ColumnVector<i64>(std::move(data)));
            meta.type = TypeId::INT64;
            meta.stats.min_i64 = min_i64;
            meta.stats.max_i64 = max_i64;
            meta.stats.ndv = uniques.size();
            table.columns.push_back(std::move(column));
            column_metas.push_back(std::move(meta));
            continue;
        }

        // Check if all values are f64
        bool all_f64 = true;
        f64 min_f64 = std::numeric_limits<f64>::max();
        f64 max_f64 = std::numeric_limits<f64>::lowest();
        for (const auto& row : rows) {
            try {
                f64 val = std::stod(row[col]);
                min_f64 = std::min(min_f64, val);
                max_f64 = std::max(max_f64, val);
            } catch (...) {
                all_f64 = false;
                break;
            }
        }
        if (all_f64 && !rows.empty()) {
            std::vector<f64> data;
            data.reserve(num_rows);
            for (const auto& row : rows) {
                data.push_back(std::stod(row[col]));
            }
            std::set<f64> uniques(data.begin(), data.end());
            column.data.reset(new ColumnVector<f64>(std::move(data)));
            meta.type = TypeId::DOUBLE;
            meta.stats.min_f64 = min_f64;
            meta.stats.max_f64 = max_f64;
            meta.stats.ndv = uniques.size();
            table.columns.push_back(std::move(column));
            column_metas.push_back(std::move(meta));
            continue;
        }

        // Else, string
        std::vector<StrId> data;
        data.reserve(num_rows);
        for (const auto& row : rows) {
            data.push_back(table.dict->get_or_add(row[col]));
        }
        std::set<StrId> uniques(data.begin(), data.end());
        column.data.reset(new ColumnVector<StrId>(std::move(data)));
        meta.stats.ndv = uniques.size(); // NDV for strings
        table.columns.push_back(std::move(column));
        column_metas.push_back(std::move(meta));
    }

    TableMeta table_meta("", std::move(column_metas), num_rows);
    return std::make_pair(std::move(table), std::move(table_meta));
}