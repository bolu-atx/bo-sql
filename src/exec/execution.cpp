#include "exec/operator.hpp"

namespace bosql {

void run_query(std::unique_ptr<Operator> root) {
    root->open();
    ExecBatch batch;
    while (root->next(batch)) {
        for (size_t i = 0; i < batch.length; ++i) {
            for (size_t j = 0; j < batch.columns.size(); ++j) {
                switch (batch.types[j]) {
                    case TypeId::INT64: {
                        const int64_t* col = reinterpret_cast<const int64_t*>(batch.columns[j]);
                        std::cout << col[i];
                        break;
                    }
                    case TypeId::DOUBLE: {
                        const double* col = reinterpret_cast<const double*>(batch.columns[j]);
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