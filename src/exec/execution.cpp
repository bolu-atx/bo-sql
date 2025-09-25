#include "exec/operator.hpp"
#include <vector>
#include <string>
#include <iostream>

namespace bosql {

void run_query(std::unique_ptr<Operator> root, const std::vector<std::string>& col_names, const std::vector<TypeId>& col_types, const Dictionary* dict) {
    std::vector<std::vector<std::string>> rows;
    root->open();
    ExecBatch batch;
    while (root->next(batch)) {
        for (size_t i = 0; i < batch.length; ++i) {
            std::vector<std::string> row;
            for (size_t j = 0; j < batch.columns.size(); ++j) {
                auto& slice = batch.columns[j];
                std::string value;
                switch (slice.type) {
                    case TypeId::INT64: {
                        auto col = get_col<int64_t>(batch, j);
                        value = std::to_string(col[i]);
                        break;
                    }
                    case TypeId::DOUBLE: {
                        auto col = get_col<double>(batch, j);
                        value = std::to_string(col[i]);
                        break;
                    }
                    case TypeId::STRING: {
                        auto col = get_col<uint32_t>(batch, j);
                        if (dict) {
                            value = dict->get(col[i]);
                        } else {
                            value = std::to_string(col[i]); // fallback to code
                        }
                        break;
                    }
                    case TypeId::DATE32: {
                        auto col = get_col<int32_t>(batch, j);
                        // For now, print as int
                        value = std::to_string(col[i]);
                        break;
                    }
                    default:
                        value = "?";
                        break;
                }
                row.push_back(value);
            }
            rows.push_back(std::move(row));
        }
    }
    root->close();

    // Print as markdown table
    if (rows.empty()) {
        std::cout << "(no results)\n";
        return;
    }

    // Header with types
    std::cout << "|";
    for (size_t j = 0; j < col_names.size(); ++j) {
        std::cout << " " << col_names[j] << " |";
    }
    std::cout << "\n";

    // Separator
    std::cout << "|";
    for (size_t j = 0; j < col_names.size(); ++j) {
        std::cout << " --- |";
    }
    std::cout << "\n";

    // Rows
    for (const auto& row : rows) {
        std::cout << "|";
        for (const auto& cell : row) {
            std::cout << " " << cell << " |";
        }
        std::cout << "\n";
    }
}

} // namespace bosql