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
    BINARY_OP,
    FUNC_CALL
};

// Binary operators
enum class BinaryOp {
    EQ, NE, LT, LE, GT, GE,
    ADD, SUB, MUL, DIV,
    AND, OR
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
    // For function calls
    std::string func_name;
    std::vector<std::unique_ptr<Expr> > args;

    std::string to_string() const;
    std::unique_ptr<Expr> clone() const;
};

// Item in SELECT list with optional alias
struct SelectItem {
    std::string alias;
    std::unique_ptr<Expr> expr;

    std::string to_string() const;
};

// Aggregate functions
enum class AggFunc {
    NONE, SUM, COUNT, AVG
};

// GROUP BY clause
struct GroupByClause {
    std::vector<std::unique_ptr<Expr> > columns;
    std::unique_ptr<Expr> having;

    std::string to_string() const;
};

// Item in ORDER BY clause
struct OrderByItem {
    std::unique_ptr<Expr> expr;
    bool asc = true;

    std::string to_string() const;
};

// Table reference with optional alias
struct TableRef {
    std::string table_name;
    std::string alias; // empty if no alias

    std::string to_string() const;
};

// JOIN clause item
struct JoinItem {
    TableRef table_ref;
    std::unique_ptr<Expr> on_condition;

    std::string to_string() const;
};

// SELECT statement AST node
struct SelectStmt {
    std::vector<SelectItem> select_list;
    TableRef from_table;
    std::unique_ptr<Expr> where_clause;
    std::vector<JoinItem> joins;
    GroupByClause group_by;
    std::vector<OrderByItem> order_by;
    int limit = -1;

    std::string to_string() const;
};