#include "logical/planner.h"
#include <set>
#include <algorithm>

// Helper to collect column names from an expression
void collect_columns(const Expr* expr, std::set<std::string>& columns) {
    if (!expr) return;
    switch (expr->type) {
        case ExprType::COLUMN_REF:
            columns.insert(expr->str_val);
            break;
        case ExprType::BINARY_OP:
            collect_columns(expr->left.get(), columns);
            collect_columns(expr->right.get(), columns);
            break;
        case ExprType::FUNC_CALL:
            for (const auto& arg : expr->args) {
                collect_columns(arg.get(), columns);
            }
            break;
        default:
            break;
    }
}

// Collect all columns referenced in the SelectStmt
std::vector<std::string> collect_all_columns(const SelectStmt& stmt) {
    std::set<std::string> columns;

    // From select_list
    for (const auto& item : stmt.select_list) {
        collect_columns(item.expr.get(), columns);
    }

    // From where
    collect_columns(stmt.where_clause.get(), columns);

    // From joins
    for (const auto& join : stmt.joins) {
        collect_columns(join.on_condition.get(), columns);
    }

    // From group_by
    for (const auto& col : stmt.group_by.columns) {
        collect_columns(col.get(), columns);
    }

    // From order_by
    for (const auto& order : stmt.order_by) {
        collect_columns(order.expr.get(), columns);
    }

    std::vector<std::string> result(columns.begin(), columns.end());
    std::sort(result.begin(), result.end());
    return result;
}

// Build the base relation (scan or join tree)
std::unique_ptr<LogicalOp> build_base_relation(const SelectStmt& stmt, const std::vector<std::string>& columns) {
    if (stmt.joins.empty()) {
        // Simple scan
        return std::make_unique<LogicalScan>(stmt.from_table.table_name, columns);
    } else {
        // For now, assume one join
        // TODO: handle multiple joins
        const auto& join = stmt.joins[0];
        auto left = std::make_unique<LogicalScan>(stmt.from_table.table_name, columns);
        auto right = std::make_unique<LogicalScan>(join.table_ref.table_name, columns);

        // Parse join condition for keys
        // Assume on_condition is col1 = col2
        std::vector<std::string> left_keys, right_keys;
        if (join.on_condition->type == ExprType::BINARY_OP && join.on_condition->op == BinaryOp::EQ) {
            if (join.on_condition->left->type == ExprType::COLUMN_REF &&
                join.on_condition->right->type == ExprType::COLUMN_REF) {
                left_keys.push_back(join.on_condition->left->str_val);
                right_keys.push_back(join.on_condition->right->str_val);
            }
        }

        auto join_op = std::make_unique<LogicalHashJoin>(left_keys, right_keys);
        join_op->children.push_back(std::move(left));
        join_op->children.push_back(std::move(right));
        return join_op;
    }
}

// Extract aggregates from select_list
std::vector<LogicalAggregate::AggExpr> extract_aggregates(const std::vector<SelectItem>& select_list) {
    std::vector<LogicalAggregate::AggExpr> aggs;
    for (const auto& item : select_list) {
        if (item.expr->type == ExprType::FUNC_CALL) {
            const auto& func = item.expr->func_name;
            if (func == "SUM" || func == "COUNT" || func == "AVG") {
                LogicalAggregate::AggExpr agg;
                agg.func_name = func;
                agg.arg = item.expr->args[0]->clone();
                agg.alias = item.alias;
                aggs.emplace_back(std::move(agg));
            }
        }
    }
    return aggs;
}

std::unique_ptr<LogicalOp> LogicalPlanner::build_logical_plan(const SelectStmt& stmt) {
    auto columns = collect_all_columns(stmt);
    auto base = build_base_relation(stmt, columns);

    // Add filter if where
    if (stmt.where_clause) {
        auto filter = std::make_unique<LogicalFilter>(stmt.where_clause->clone()); // TODO: clone
        filter->children.push_back(std::move(base));
        base = std::move(filter);
    }

    // Add aggregate if group by
    if (!stmt.group_by.columns.empty()) {
        auto aggs = extract_aggregates(stmt.select_list);
        std::vector<std::unique_ptr<Expr>> group_keys;
        for (const auto& col : stmt.group_by.columns) {
            group_keys.push_back(col->clone()); // TODO: clone
        }
        auto agg = std::make_unique<LogicalAggregate>(std::move(group_keys), std::move(aggs));
        agg->children.push_back(std::move(base));
        base = std::move(agg);
    }

    // Add project
    std::vector<std::unique_ptr<Expr>> select_exprs;
    std::vector<std::string> aliases;
    for (const auto& item : stmt.select_list) {
        select_exprs.push_back(item.expr->clone()); // TODO: clone
        aliases.push_back(item.alias);
    }
    auto project = std::make_unique<LogicalProject>(std::move(select_exprs), std::move(aliases));
    project->children.push_back(std::move(base));
    base = std::move(project);

    // Add order if order_by
    if (!stmt.order_by.empty()) {
        std::vector<LogicalOrder::OrderItem> order_items;
        for (const auto& order : stmt.order_by) {
            LogicalOrder::OrderItem item;
            item.expr = order.expr->clone();
            item.asc = order.asc;
            order_items.emplace_back(std::move(item));
        }
        auto order_op = std::make_unique<LogicalOrder>(std::move(order_items));
        order_op->children.push_back(std::move(base));
        base = std::move(order_op);
    }

    // Add limit if limit
    if (stmt.limit >= 0) {
        auto limit_op = std::make_unique<LogicalLimit>(stmt.limit);
        limit_op->children.push_back(std::move(base));
        base = std::move(limit_op);
    }

    return base;
}