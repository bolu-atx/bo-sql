#pragma once

#include <string>
#include <vector>
#include <variant>
#include <memory>
#include "types.h"

// Types of expressions in the AST
enum class ExprType {
    COLUMN_REF,
    LITERAL_INT,
    LITERAL_DOUBLE,
    LITERAL_STRING,
    BINARY_OP
};

// Binary operators
enum class BinaryOp {
    EQ, NE, LT, LE, GT, GE,
    ADD, SUB, MUL, DIV
};

// Base expression node
struct Expr {
    ExprType type;
    // For literals: use separate fields since no variant
    std::string str_val;
    i64 i64_val;
    f64 f64_val;
    BinaryOp op; // for binary
    std::unique_ptr<Expr> left, right; // for binary
};

// Item in SELECT list with optional alias
struct SelectItem {
    std::string alias;
    std::unique_ptr<Expr> expr;
};

// Aggregate functions
enum class AggFunc {
    NONE, SUM, COUNT, AVG
};

// GROUP BY clause
struct GroupByClause {
    std::vector<std::unique_ptr<Expr> > columns;
};

// Item in ORDER BY clause
struct OrderByItem {
    std::unique_ptr<Expr> expr;
    bool asc = true;
};

// SELECT statement AST node
struct SelectStmt {
    std::vector<SelectItem> select_list;
    std::string from_table;
    std::unique_ptr<Expr> where_clause;
    std::vector<std::pair<std::string, std::string> > joins; // table, on_condition as string for simplicity
    GroupByClause group_by;
    std::vector<OrderByItem> order_by;
    int limit = -1;
};