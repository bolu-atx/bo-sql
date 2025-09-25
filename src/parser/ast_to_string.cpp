#include "parser/ast.h"
#include "parser/ast.h"
#include <fmt/core.h>
#include <sstream>

namespace bosql {

std::string Expr::to_string() const {
    switch (type) {
        case ExprType::COLUMN_REF:
            return str_val;
        case ExprType::LITERAL_INT:
            return fmt::format("{}", i64_val);
        case ExprType::LITERAL_DOUBLE:
            return fmt::format("{}", f64_val);
        case ExprType::LITERAL_STRING:
            return fmt::format("'{}'", str_val);
        case ExprType::BINARY_OP: {
            std::string op_str;
            switch (op) {
                case BinaryOp::EQ: op_str = "="; break;
                case BinaryOp::NE: op_str = "!="; break;
                case BinaryOp::LT: op_str = "<"; break;
                case BinaryOp::LE: op_str = "<="; break;
                case BinaryOp::GT: op_str = ">"; break;
                case BinaryOp::GE: op_str = ">="; break;
                case BinaryOp::ADD: op_str = "+"; break;
                case BinaryOp::SUB: op_str = "-"; break;
                case BinaryOp::MUL: op_str = "*"; break;
                case BinaryOp::DIV: op_str = "/"; break;
                case BinaryOp::AND: op_str = "AND"; break;
                case BinaryOp::OR: op_str = "OR"; break;
            }
            return fmt::format("({} {} {})", left->to_string(), op_str, right->to_string());
        }
        case ExprType::FUNC_CALL: {
            std::string args_str;
            for (size_t i = 0; i < args.size(); ++i) {
                if (i > 0) args_str += ", ";
                args_str += args[i]->to_string();
            }
            return fmt::format("{}({})", func_name, args_str);
        }
        default:
            return "UNKNOWN_EXPR";
    }
}

std::string SelectItem::to_string() const {
    if (!alias.empty()) {
        return fmt::format("{} AS {}", expr->to_string(), alias);
    }
    return expr->to_string();
}

std::string GroupByClause::to_string() const {
    if (columns.empty()) return "";

    std::string result = "GROUP BY ";
    for (size_t i = 0; i < columns.size(); ++i) {
        if (i > 0) result += ", ";
        result += columns[i]->to_string();
    }
    if (having) {
        result += fmt::format(" HAVING {}", having->to_string());
    }
    return result;
}

std::string OrderByItem::to_string() const {
    return fmt::format("{} {}", expr->to_string(), asc ? "ASC" : "DESC");
}

std::string TableRef::to_string() const {
    if (alias.empty()) {
        return table_name;
    } else {
        return fmt::format("{} {}", table_name, alias);
    }
}

std::string JoinItem::to_string() const {
    return fmt::format("JOIN {} ON {}", table_ref.to_string(), on_condition->to_string());
}

std::string SelectStmt::to_string() const {
    std::stringstream ss;

    ss << "SELECT ";
    for (size_t i = 0; i < select_list.size(); ++i) {
        if (i > 0) ss << ", ";
        ss << select_list[i].to_string();
    }

    ss << " FROM " << from_table.to_string();

    for (const auto& join : joins) {
        ss << " " << join.to_string();
    }

    if (where_clause) {
        ss << " WHERE " << where_clause->to_string();
    }

    if (!group_by.columns.empty()) {
        ss << " " << group_by.to_string();
    }

    if (!order_by.empty()) {
        ss << " ORDER BY ";
        for (size_t i = 0; i < order_by.size(); ++i) {
            if (i > 0) ss << ", ";
            ss << order_by[i].to_string();
        }
    }

    if (limit >= 0) {
        ss << " LIMIT " << limit;
    }

    return ss.str();
}

std::unique_ptr<Expr> Expr::clone() const {
    auto cloned = std::make_unique<Expr>();
    cloned->type = type;
    cloned->str_val = str_val;
    cloned->i64_val = i64_val;
    cloned->f64_val = f64_val;
    cloned->op = op;
    if (left) cloned->left = left->clone();
    if (right) cloned->right = right->clone();
    cloned->func_name = func_name;
    for (const auto& arg : args) {
        cloned->args.push_back(arg->clone());
    }
    return cloned;
}

} // namespace bosql