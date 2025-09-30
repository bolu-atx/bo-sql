#include "exec/physical_planner.h"
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
            auto child = build_physical_plan(project->children[0].get(), catalog);
            if (project->select_list.empty()) {
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
