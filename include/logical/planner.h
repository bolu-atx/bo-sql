#pragma once

#include <memory>
#include <utility>
#include <vector>
#include <tuple>
#include "parser/ast.h"
#include "logical/logical.h"
#include "catalog/catalog.h"
#include "storage/dictionary.h"

namespace bosql {

class LogicalPlanner {
public:
    std::unique_ptr<LogicalOp> build_logical_plan(const SelectStmt& stmt);
};

// Get output schema (column names, types, and dictionary) from logical plan
std::tuple<std::vector<std::string>, std::vector<TypeId>, const Dictionary*> get_output_schema(const LogicalOp* plan, const Catalog& catalog);

} // namespace bosql