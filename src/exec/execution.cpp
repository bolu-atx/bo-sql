#include "exec/operator.hpp"
#include <vector>
#include <string>
#include <iostream>

namespace bosql {

void run_query(std::unique_ptr<Operator> root,
               const std::vector<std::string>& col_names,
               const std::vector<TypeId>& col_types,
               Formatter& formatter,
               const Dictionary* dict) {
    formatter.begin(col_names, col_types);
    root->open();
    ExecBatch batch;
    std::size_t row_count = 0;
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
            formatter.write_row(std::move(row));
            ++row_count;
        }
    }
    root->close();
    formatter.end(row_count);
}

} // namespace bosql
