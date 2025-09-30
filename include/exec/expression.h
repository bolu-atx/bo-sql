#pragma once

#include <unordered_map>
#include <string>
#include <vector>
#include "types.h"
#include "exec/execution_types.hpp"
#include "storage/dictionary.h"
#include "parser/ast.h"

namespace bosql {

struct ExprBindings {
    const std::vector<std::string>* column_names;
    const std::vector<TypeId>* column_types;
    std::unordered_map<std::string, size_t> name_to_index;
    Dictionary* dictionary = nullptr;
};

ExprBindings make_bindings(const std::vector<std::string>& names,
                           const std::vector<TypeId>& types,
                           Dictionary* dictionary = nullptr);

Datum evaluate_expr(const Expr* expr,
                    const ExecBatch& batch,
                    size_t row,
                    const ExprBindings& bindings);

bool evaluate_predicate(const Expr* expr,
                        const ExecBatch& batch,
                        size_t row,
                        const ExprBindings& bindings);

}
