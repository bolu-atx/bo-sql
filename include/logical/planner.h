#pragma once

#include <memory>
#include "parser/ast.h"
#include "logical/logical.h"

class LogicalPlanner {
public:
    std::unique_ptr<LogicalOp> build_logical_plan(const SelectStmt& stmt);
};