#include "exec/expression.h"
#include <stdexcept>
#include <cmath>
#include <limits>

namespace bosql {

namespace {

bool is_truthy(const Datum& value) {
    switch (value.type) {
        case TypeId::INT64:
            return value.value.i64_val != 0;
        case TypeId::DOUBLE:
            return value.value.f64_val != 0.0;
        case TypeId::STRING:
            return value.value.str_id != 0;
        case TypeId::DATE32:
            return value.value.date32_val != 0;
    }
    return false;
}

Datum coerce_numeric(const Datum& value) {
    if (value.type == TypeId::STRING) {
        throw std::runtime_error("Cannot coerce string to numeric");
    }
    return value;
}

Datum numeric_binary(const Datum& left, const Datum& right, BinaryOp op) {
    Datum lhs = coerce_numeric(left);
    Datum rhs = coerce_numeric(right);
    if (lhs.type == TypeId::DOUBLE || rhs.type == TypeId::DOUBLE) {
        double l = lhs.type == TypeId::DOUBLE ? lhs.value.f64_val : static_cast<double>(lhs.value.i64_val);
        double r = rhs.type == TypeId::DOUBLE ? rhs.value.f64_val : static_cast<double>(rhs.value.i64_val);
        switch (op) {
            case BinaryOp::ADD: return Datum::from_f64(l + r);
            case BinaryOp::SUB: return Datum::from_f64(l - r);
            case BinaryOp::MUL: return Datum::from_f64(l * r);
            case BinaryOp::DIV: return Datum::from_f64(r == 0.0 ? std::numeric_limits<double>::infinity() : l / r);
            default: break;
        }
    } else {
        int64_t l = lhs.value.i64_val;
        int64_t r = rhs.value.i64_val;
        switch (op) {
            case BinaryOp::ADD: return Datum::from_i64(l + r);
            case BinaryOp::SUB: return Datum::from_i64(l - r);
            case BinaryOp::MUL: return Datum::from_i64(l * r);
            case BinaryOp::DIV:
                if (r == 0) throw std::runtime_error("Division by zero");
                return Datum::from_i64(l / r);
            default: break;
        }
    }
    throw std::runtime_error("Unsupported arithmetic operator");
}

Datum compare_values(const Datum& left, const Datum& right, BinaryOp op) {
    switch (left.type) {
        case TypeId::INT64: {
            int64_t l = left.value.i64_val;
            int64_t r = right.type == TypeId::INT64 ? right.value.i64_val : static_cast<int64_t>(right.value.f64_val);
            bool result = false;
            switch (op) {
                case BinaryOp::EQ: result = (l == r); break;
                case BinaryOp::NE: result = (l != r); break;
                case BinaryOp::LT: result = (l < r); break;
                case BinaryOp::LE: result = (l <= r); break;
                case BinaryOp::GT: result = (l > r); break;
                case BinaryOp::GE: result = (l >= r); break;
                default: throw std::runtime_error("Invalid comparison operator");
            }
            return Datum::from_i64(result ? 1 : 0);
        }
        case TypeId::DOUBLE: {
            double l = left.value.f64_val;
            double r = right.type == TypeId::DOUBLE ? right.value.f64_val : static_cast<double>(right.value.i64_val);
            bool result = false;
            switch (op) {
                case BinaryOp::EQ: result = (l == r); break;
                case BinaryOp::NE: result = (l != r); break;
                case BinaryOp::LT: result = (l < r); break;
                case BinaryOp::LE: result = (l <= r); break;
                case BinaryOp::GT: result = (l > r); break;
                case BinaryOp::GE: result = (l >= r); break;
                default: throw std::runtime_error("Invalid comparison operator");
            }
            return Datum::from_i64(result ? 1 : 0);
        }
        case TypeId::DATE32: {
            int32_t l = left.value.date32_val;
            int32_t r = right.value.date32_val;
            bool result = false;
            switch (op) {
                case BinaryOp::EQ: result = (l == r); break;
                case BinaryOp::NE: result = (l != r); break;
                case BinaryOp::LT: result = (l < r); break;
                case BinaryOp::LE: result = (l <= r); break;
                case BinaryOp::GT: result = (l > r); break;
                case BinaryOp::GE: result = (l >= r); break;
                default: throw std::runtime_error("Invalid comparison operator");
            }
            return Datum::from_i64(result ? 1 : 0);
        }
        case TypeId::STRING: {
            StrId l = left.value.str_id;
            StrId r = right.value.str_id;
            bool result = false;
            switch (op) {
                case BinaryOp::EQ: result = (l == r); break;
                case BinaryOp::NE: result = (l != r); break;
                default: throw std::runtime_error("Unsupported string comparison");
            }
            return Datum::from_i64(result ? 1 : 0);
        }
    }
    throw std::runtime_error("Unknown type in comparison");
}

Datum evaluate_internal(const Expr* expr,
                        const ExecBatch& batch,
                        size_t row,
                        const ExprBindings& bindings);

Datum read_column(size_t index,
                  const ExecBatch& batch,
                  size_t row,
                  const std::vector<TypeId>& types) {
    const auto& slice = batch.columns[index];
    switch (types[index]) {
        case TypeId::INT64: {
            auto ptr = reinterpret_cast<const int64_t*>(slice.data);
            return Datum::from_i64(ptr[row]);
        }
        case TypeId::DOUBLE: {
            auto ptr = reinterpret_cast<const double*>(slice.data);
            return Datum::from_f64(ptr[row]);
        }
        case TypeId::STRING: {
            auto ptr = reinterpret_cast<const uint32_t*>(slice.data);
            return Datum::from_str(ptr[row]);
        }
        case TypeId::DATE32: {
            auto ptr = reinterpret_cast<const int32_t*>(slice.data);
            return Datum::from_date32(ptr[row]);
        }
    }
    throw std::runtime_error("Unknown column type");
}

Datum evaluate_internal(const Expr* expr,
                        const ExecBatch& batch,
                        size_t row,
                        const ExprBindings& bindings) {
    switch (expr->type) {
        case ExprType::COLUMN_REF: {
            auto it = bindings.name_to_index.find(expr->str_val);
            if (it == bindings.name_to_index.end()) {
                throw std::runtime_error("Unknown column: " + expr->str_val);
            }
            return read_column(it->second, batch, row, *bindings.column_types);
        }
        case ExprType::LITERAL_INT:
            return Datum::from_i64(expr->i64_val);
        case ExprType::LITERAL_DOUBLE:
            return Datum::from_f64(expr->f64_val);
        case ExprType::LITERAL_STRING: {
            if (!bindings.dictionary) {
                throw std::runtime_error("String literal without dictionary binding");
            }
            return Datum::from_str(bindings.dictionary->get_or_add(expr->str_val));
        }
        case ExprType::BINARY_OP: {
            Datum left = evaluate_internal(expr->left.get(), batch, row, bindings);
            Datum right = evaluate_internal(expr->right.get(), batch, row, bindings);
            switch (expr->op) {
                case BinaryOp::ADD:
                case BinaryOp::SUB:
                case BinaryOp::MUL:
                case BinaryOp::DIV:
                    return numeric_binary(left, right, expr->op);
                case BinaryOp::EQ:
                case BinaryOp::NE:
                case BinaryOp::LT:
                case BinaryOp::LE:
                case BinaryOp::GT:
                case BinaryOp::GE:
                    return compare_values(left, right, expr->op);
                case BinaryOp::AND: {
                    bool result = is_truthy(left) && is_truthy(right);
                    return Datum::from_i64(result ? 1 : 0);
                }
                case BinaryOp::OR: {
                    bool result = is_truthy(left) || is_truthy(right);
                    return Datum::from_i64(result ? 1 : 0);
                }
            }
            throw std::runtime_error("Unsupported binary operator");
        }
        case ExprType::FUNC_CALL:
            throw std::runtime_error("Function calls not supported in expression evaluation");
    }
    throw std::runtime_error("Unknown expression type");
}

}

ExprBindings make_bindings(const std::vector<std::string>& names,
                           const std::vector<TypeId>& types,
                           Dictionary* dictionary) {
    ExprBindings bindings;
    bindings.column_names = &names;
    bindings.column_types = &types;
    bindings.dictionary = dictionary;
    for (size_t i = 0; i < names.size(); ++i) {
        bindings.name_to_index[names[i]] = i;
    }
    return bindings;
}

Datum evaluate_expr(const Expr* expr,
                    const ExecBatch& batch,
                    size_t row,
                    const ExprBindings& bindings) {
    return evaluate_internal(expr, batch, row, bindings);
}

bool evaluate_predicate(const Expr* expr,
                        const ExecBatch& batch,
                        size_t row,
                        const ExprBindings& bindings) {
    Datum value = evaluate_internal(expr, batch, row, bindings);
    return is_truthy(value);
}

}
