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
            return std::make_unique<ColumnarScan>(const_cast<Table*>(&table.value()));
        }
        case LogicalOpType::FILTER: {
            const auto* filter = dynamic_cast<const LogicalFilter*>(logical);
            if (!filter) throw std::runtime_error("Invalid LogicalFilter");
            auto child = build_physical_plan(filter->children[0].get(), catalog);
            return std::make_unique<Selection>(std::move(child));
        }
        case LogicalOpType::PROJECT: {
            const auto* project = dynamic_cast<const LogicalProject*>(logical);
            if (!project) throw std::runtime_error("Invalid LogicalProject");
            auto child = build_physical_plan(project->children[0].get(), catalog);
            // For MVP, assume select all columns or first
            std::vector<int> indices;
            for (size_t i = 0; i < project->select_list.size(); ++i) {
                // Assume column refs, get index
                // For MVP, assume columns are 0,1,2...
                indices.push_back(i);
            }
            return std::make_unique<Project>(std::move(child), indices);
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

} // namespace bosql