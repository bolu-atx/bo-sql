#include "exec/operator.hpp"

namespace bosql {

void run_query(std::unique_ptr<Operator> root) {
    root->open();
    ExecBatch batch;
    while (root->next(batch)) {
        for (size_t i = 0; i < batch.length; ++i) {
            for (size_t j = 0; j < batch.columns.size(); ++j) {
                auto& slice = batch.columns[j];
                switch (slice.type) {
                    case TypeId::INT64: {
                        auto col = get_col<int64_t>(batch, j);
                        std::cout << col[i];
                        break;
                    }
                    case TypeId::DOUBLE: {
                        auto col = get_col<double>(batch, j);
                        std::cout << col[i];
                        break;
                    }
                    // Add others
                    default:
                        std::cout << "?";
                        break;
                }
                if (j + 1 < batch.columns.size()) std::cout << '\t';
            }
            std::cout << '\n';
        }
    }
    root->close();
}

} // namespace bosql