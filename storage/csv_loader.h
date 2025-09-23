#pragma once

#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <algorithm>
#include "../include/types.h"
#include "table.h"

Table load_csv(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open file: " + filename);
    }

    Table table;
    table.dict = std::make_shared<Dictionary>();

    std::string line;
    std::vector<std::string> headers;
    std::vector<std::vector<std::string>> rows;

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
        Column column;
        column.name = col_name;

        // Check if all values are i64
        bool all_i64 = true;
        for (const auto& row : rows) {
            try {
                std::stoll(row[col]);
            } catch (...) {
                all_i64 = false;
                break;
            }
        }
        if (all_i64 && !rows.empty()) {
            ColumnVector<i64> vec;
            vec.data.reserve(num_rows);
            for (const auto& row : rows) {
                vec.data.push_back(std::stoll(row[col]));
            }
            column.data = std::move(vec);
            table.columns.push_back(std::move(column));
            continue;
        }

        // Check if all values are f64
        bool all_f64 = true;
        for (const auto& row : rows) {
            try {
                std::stod(row[col]);
            } catch (...) {
                all_f64 = false;
                break;
            }
        }
        if (all_f64 && !rows.empty()) {
            ColumnVector<f64> vec;
            vec.data.reserve(num_rows);
            for (const auto& row : rows) {
                vec.data.push_back(std::stod(row[col]));
            }
            column.data = std::move(vec);
            table.columns.push_back(std::move(column));
            continue;
        }

        // Check if all values are date (8 digits)
        bool all_date = true;
        for (const auto& row : rows) {
            if (row[col].size() != 8) {
                all_date = false;
                break;
            }
            try {
                int d = std::stoi(row[col]);
                if (d < 19000000 || d > 21000000) all_date = false;
            } catch (...) {
                all_date = false;
                break;
            }
        }
        if (all_date && !rows.empty()) {
            ColumnVector<Date32> vec;
            vec.data.reserve(num_rows);
            for (const auto& row : rows) {
                vec.data.push_back(std::stoi(row[col]));
            }
            column.data = std::move(vec);
            table.columns.push_back(std::move(column));
            continue;
        }

        // Else, string
        ColumnVector<StrId> vec;
        vec.data.reserve(num_rows);
        for (const auto& row : rows) {
            vec.data.push_back(table.dict->get_or_add(row[col]));
        }
        column.data = std::move(vec);
        table.columns.push_back(std::move(column));
    }

    return table;
}