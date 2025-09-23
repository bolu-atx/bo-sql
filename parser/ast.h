#pragma once

#include <string>
#include <vector>
#include <variant>
#include <memory>
#include "../include/types.h"

enum class ExprType {
    COLUMN_REF,
    LITERAL_INT,
    LITERAL_DOUBLE,
    LITERAL_STRING,
    BINARY_OP
};

enum class BinaryOp {
    EQ, NE, LT, LE, GT, GE,
    ADD, SUB, MUL, DIV
};

struct Expr {
    ExprType type;
    std::variant<std::string, i64, f64, std::string> value; // for literals
    BinaryOp op; // for binary
    std::unique_ptr<Expr> left, right; // for binary
};

struct SelectItem {
    std::string alias;
    std::unique_ptr<Expr> expr;
};

enum class AggFunc {
    NONE, SUM, COUNT, AVG
};

struct GroupByClause {
    std::vector<std::unique_ptr<Expr> > columns;
};

struct OrderByItem {
    std::unique_ptr<Expr> expr;
    bool asc = true;
};

struct SelectStmt {
    std::vector<SelectItem> select_list;
    std::string from_table;
    std::unique_ptr<Expr> where_clause;
    std::vector<std::pair<std::string, std::string> > joins; // table, on_condition as string for simplicity
    GroupByClause group_by;
    std::vector<OrderByItem> order_by;
    int limit = -1;
};