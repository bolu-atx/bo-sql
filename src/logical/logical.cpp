#include "logical/logical.h"
#include <fmt/core.h>
#include <sstream>

std::string LogicalScan::to_string(int indent) const {
    std::string prefix(indent, ' ');
    std::string cols_str;
    for (size_t i = 0; i < columns.size(); ++i) {
        if (i > 0) cols_str += ", ";
        cols_str += columns[i];
    }
    std::string result = fmt::format("{}LogicalScan(table={}, cols={})", prefix, table_name, cols_str);
    if (!children.empty()) {
        // Scan shouldn't have children, but just in case
        for (const auto& child : children) {
            result += "\n" + child->to_string(indent + 2);
        }
    }
    return result;
}

std::string LogicalFilter::to_string(int indent) const {
    std::string prefix(indent, ' ');
    std::string result = fmt::format("{}LogicalFilter({})", prefix, predicate->to_string());
    for (const auto& child : children) {
        result += "\n" + child->to_string(indent + 2);
    }
    return result;
}

std::string LogicalProject::to_string(int indent) const {
    std::string prefix(indent, ' ');
    std::string selects_str;
    for (size_t i = 0; i < select_list.size(); ++i) {
        if (i > 0) selects_str += ", ";
        selects_str += select_list[i]->to_string();
        if (!aliases[i].empty()) {
            selects_str += fmt::format(" AS {}", aliases[i]);
        }
    }
    std::string result = fmt::format("{}LogicalProject({})", prefix, selects_str);
    for (const auto& child : children) {
        result += "\n" + child->to_string(indent + 2);
    }
    return result;
}

std::string LogicalHashJoin::to_string(int indent) const {
    std::string prefix(indent, ' ');
    std::string lkeys_str;
    for (size_t i = 0; i < left_keys.size(); ++i) {
        if (i > 0) lkeys_str += ", ";
        lkeys_str += left_keys[i];
    }
    std::string rkeys_str;
    for (size_t i = 0; i < right_keys.size(); ++i) {
        if (i > 0) rkeys_str += ", ";
        rkeys_str += right_keys[i];
    }
    std::string result = fmt::format("{}LogicalHashJoin(left_keys={}, right_keys={}", prefix, lkeys_str, rkeys_str);
    if (join_filter) {
        result += fmt::format(", filter={}", join_filter->to_string());
    }
    result += ")";
    for (const auto& child : children) {
        result += "\n" + child->to_string(indent + 2);
    }
    return result;
}

std::string LogicalAggregate::to_string(int indent) const {
    std::string prefix(indent, ' ');
    std::string keys_str;
    for (size_t i = 0; i < group_keys.size(); ++i) {
        if (i > 0) keys_str += ", ";
        keys_str += group_keys[i]->to_string();
    }
    std::string aggs_str;
    for (size_t i = 0; i < aggregates.size(); ++i) {
        if (i > 0) aggs_str += ", ";
        aggs_str += fmt::format("{}({})", aggregates[i].func_name, aggregates[i].arg->to_string());
        if (!aggregates[i].alias.empty()) {
            aggs_str += fmt::format(" AS {}", aggregates[i].alias);
        }
    }
    std::string result = fmt::format("{}LogicalAggregate(keys={}, aggs={})", prefix, keys_str, aggs_str);
    for (const auto& child : children) {
        result += "\n" + child->to_string(indent + 2);
    }
    return result;
}

std::string LogicalOrder::to_string(int indent) const {
    std::string prefix(indent, ' ');
    std::string order_str;
    for (size_t i = 0; i < order_by.size(); ++i) {
        if (i > 0) order_str += ", ";
        order_str += fmt::format("{} {}", order_by[i].expr->to_string(), order_by[i].asc ? "ASC" : "DESC");
    }
    std::string result = fmt::format("{}LogicalOrder(by: {})", prefix, order_str);
    for (const auto& child : children) {
        result += "\n" + child->to_string(indent + 2);
    }
    return result;
}

std::string LogicalLimit::to_string(int indent) const {
    std::string prefix(indent, ' ');
    std::string result = fmt::format("{}LogicalLimit({})", prefix, limit);
    for (const auto& child : children) {
        result += "\n" + child->to_string(indent + 2);
    }
    return result;
}