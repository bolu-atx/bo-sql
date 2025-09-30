#include "exec/physical_planner.h"
#include <algorithm>
#include <cctype>
#include <iostream>
#include <stdexcept>

namespace bosql {

std::unique_ptr<Operator> build_physical_plan(const LogicalOp* logical, const Catalog& catalog) {
    switch (logical->type) {
        case LogicalOpType::SCAN: {
            const auto* scan = dynamic_cast<const LogicalScan*>(logical);
            if (!scan) throw std::runtime_error("Invalid LogicalScan");
            OptionalRef<const Table> table = catalog.get_table_data(scan->table_name);
            if (!table.has_value()) throw std::runtime_error("Table not found: " + scan->table_name);
            std::vector<size_t> indices;
            if (!scan->columns.empty()) {
                const Table& tbl = table.value();
                indices.reserve(scan->columns.size());
                for (const auto& name : scan->columns) {
                    bool matched = false;
                    for (size_t i = 0; i < tbl.columns.size(); ++i) {
                        if (tbl.columns[i].name == name) {
                            indices.push_back(i);
                            matched = true;
                            break;
                        }
                    }
                    if (!matched) {
                        continue;
                    }
                }
            }
            return std::make_unique<ColumnarScan>(const_cast<Table*>(&table.value()), std::move(indices));
        }
        case LogicalOpType::FILTER: {
            const auto* filter = dynamic_cast<const LogicalFilter*>(logical);
            if (!filter) throw std::runtime_error("Invalid LogicalFilter");
            auto child = build_physical_plan(filter->children[0].get(), catalog);
            return std::make_unique<Selection>(std::move(child), filter->predicate->clone());
        }
        case LogicalOpType::PROJECT: {
            const auto* project = dynamic_cast<const LogicalProject*>(logical);
            if (!project) throw std::runtime_error("Invalid LogicalProject");
            const LogicalOp* child_logical = project->children[0].get();
            auto child = build_physical_plan(child_logical, catalog);
            if (project->select_list.empty()) {
                return child;
            }
            if (child_logical->type == LogicalOpType::AGGREGATE) {
                return child;
            }
            std::vector<std::unique_ptr<Expr>> exprs;
            exprs.reserve(project->select_list.size());
            for (const auto& expr : project->select_list) {
                exprs.push_back(expr->clone());
            }
            auto aliases = project->aliases;
            return std::make_unique<Project>(std::move(child), std::move(exprs), std::move(aliases));
        }
        case LogicalOpType::HASH_JOIN: {
            const auto* join = dynamic_cast<const LogicalHashJoin*>(logical);
            if (!join) throw std::runtime_error("Invalid LogicalHashJoin");
            auto left = build_physical_plan(join->children[0].get(), catalog);
            auto right = build_physical_plan(join->children[1].get(), catalog);
            std::unique_ptr<Expr> residual;
            if (join->join_filter) {
                residual = join->join_filter->clone();
            }
            return std::make_unique<HashJoin>(std::move(left),
                                             std::move(right),
                                             join->left_keys,
                                             join->right_keys,
                                             std::move(residual));
        }
        case LogicalOpType::AGGREGATE: {
            const auto* aggregate = dynamic_cast<const LogicalAggregate*>(logical);
            if (!aggregate) throw std::runtime_error("Invalid LogicalAggregate");
            auto child = build_physical_plan(aggregate->children[0].get(), catalog);
            std::vector<std::unique_ptr<Expr>> group_exprs;
            group_exprs.reserve(aggregate->group_keys.size());
            for (const auto& key : aggregate->group_keys) {
                group_exprs.push_back(key->clone());
            }
            std::vector<AggregateSpec> specs;
            specs.reserve(aggregate->aggregates.size());
            for (const auto& agg : aggregate->aggregates) {
                AggregateSpec spec;
                spec.func_name = agg.func_name;
                std::transform(spec.func_name.begin(), spec.func_name.end(), spec.func_name.begin(), [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
                spec.alias = agg.alias;
                if (agg.arg) {
                    spec.arg = agg.arg->clone();
                } else {
                    spec.arg.reset();
                }
                specs.push_back(std::move(spec));
            }
            return std::make_unique<HashAggregate>(std::move(child), std::move(group_exprs), std::move(specs));
        }
        case LogicalOpType::ORDER: {
            const auto* order = dynamic_cast<const LogicalOrder*>(logical);
            if (!order) throw std::runtime_error("Invalid LogicalOrder");
            auto child = build_physical_plan(order->children[0].get(), catalog);
            std::vector<OrderBy::SortKey> sort_keys;
            sort_keys.reserve(order->order_by.size());
            for (const auto& item : order->order_by) {
                OrderBy::SortKey key;
                key.expr = item.expr->clone();
                key.asc = item.asc;
                sort_keys.push_back(std::move(key));
            }
            return std::make_unique<OrderBy>(std::move(child), std::move(sort_keys));
        }
        case LogicalOpType::LIMIT: {
            const auto* limit = dynamic_cast<const LogicalLimit*>(logical);
            if (!limit) throw std::runtime_error("Invalid LogicalLimit");
            auto child = build_physical_plan(limit->children[0].get(), catalog);
            return std::make_unique<Limit>(std::move(child), limit->limit);
        }
        default:
            throw std::runtime_error("Unsupported logical operator");
    }
}

}
