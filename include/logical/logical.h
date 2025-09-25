#pragma once

#include <string>
#include <vector>
#include <memory>
#include "parser/ast.h"

namespace bosql {

// Types of logical operators
enum class LogicalOpType {
    SCAN,
    FILTER,
    PROJECT,
    HASH_JOIN,
    AGGREGATE,
    ORDER,
    LIMIT
};

// Base logical operator node
struct LogicalOp {
    LogicalOpType type;
    std::vector<std::unique_ptr<LogicalOp>> children;

    LogicalOp(LogicalOpType t) : type(t) {}
    virtual ~LogicalOp() = default;
    virtual std::string to_string(int indent = 0) const = 0;
};

// Read a base table
struct LogicalScan : LogicalOp {
    std::string table_name;
    std::vector<std::string> columns; // pruned later

    LogicalScan(const std::string& table, const std::vector<std::string>& cols)
        : LogicalOp(LogicalOpType::SCAN), table_name(table), columns(cols) {}

    std::string to_string(int indent = 0) const override;
};

// Restrict rows
struct LogicalFilter : LogicalOp {
    std::unique_ptr<Expr> predicate;

    LogicalFilter(std::unique_ptr<Expr> pred)
        : LogicalOp(LogicalOpType::FILTER), predicate(std::move(pred)) {}

    std::string to_string(int indent = 0) const override;
};

// Compute expressions for SELECT list
struct LogicalProject : LogicalOp {
    std::vector<std::unique_ptr<Expr>> select_list;
    std::vector<std::string> aliases;

    LogicalProject(std::vector<std::unique_ptr<Expr>>&& selects, std::vector<std::string>&& alias_list)
        : LogicalOp(LogicalOpType::PROJECT), select_list(std::move(selects)), aliases(std::move(alias_list)) {}

    std::string to_string(int indent = 0) const override;
};

// Relational join (equi-joins)
struct LogicalHashJoin : LogicalOp {
    std::vector<std::string> left_keys;
    std::vector<std::string> right_keys;
    std::unique_ptr<Expr> join_filter; // optional extra predicates

    LogicalHashJoin(std::vector<std::string> lkeys, std::vector<std::string> rkeys, std::unique_ptr<Expr> filter = nullptr)
        : LogicalOp(LogicalOpType::HASH_JOIN), left_keys(std::move(lkeys)), right_keys(std::move(rkeys)), join_filter(std::move(filter)) {}

    std::string to_string(int indent = 0) const override;
};

// GROUP BY + aggregates
struct LogicalAggregate : LogicalOp {
    struct AggExpr {
        std::string func_name; // SUM, COUNT, AVG
        std::unique_ptr<Expr> arg;
        std::string alias;
    };
    std::vector<std::unique_ptr<Expr>> group_keys;
    std::vector<AggExpr> aggregates;

    LogicalAggregate(std::vector<std::unique_ptr<Expr>>&& keys, std::vector<AggExpr>&& aggs)
        : LogicalOp(LogicalOpType::AGGREGATE), group_keys(std::move(keys)), aggregates(std::move(aggs)) {}

    std::string to_string(int indent = 0) const override;
};

// ORDER BY
struct LogicalOrder : LogicalOp {
    struct OrderItem {
        std::unique_ptr<Expr> expr;
        bool asc;
    };
    std::vector<OrderItem> order_by;

    LogicalOrder(std::vector<OrderItem>&& order)
        : LogicalOp(LogicalOpType::ORDER), order_by(std::move(order)) {}

    std::string to_string(int indent = 0) const override;
};

// LIMIT N
struct LogicalLimit : LogicalOp {
    int64_t limit;

    LogicalLimit(int64_t lim)
        : LogicalOp(LogicalOpType::LIMIT), limit(lim) {}

    std::string to_string(int indent = 0) const override;
};

} // namespace bosql