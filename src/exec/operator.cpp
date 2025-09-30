#include "exec/operator.hpp"
#include "exec/expression.h"
#include <algorithm>
#include <stdexcept>

namespace bosql {

namespace {

ColumnSlice copy_selected(const ColumnSlice& slice,
                          TypeId type,
                          const std::vector<size_t>& indices) {
    switch (type) {
        case TypeId::INT64: {
            auto src = reinterpret_cast<const int64_t*>(slice.data);
            auto buffer = std::make_shared<std::vector<int64_t>>(indices.size());
            for (size_t i = 0; i < indices.size(); ++i) {
                (*buffer)[i] = src[indices[i]];
            }
            return {buffer->data(), type, indices.size(), std::shared_ptr<void>(buffer, buffer->data())};
        }
        case TypeId::DOUBLE: {
            auto src = reinterpret_cast<const double*>(slice.data);
            auto buffer = std::make_shared<std::vector<double>>(indices.size());
            for (size_t i = 0; i < indices.size(); ++i) {
                (*buffer)[i] = src[indices[i]];
            }
            return {buffer->data(), type, indices.size(), std::shared_ptr<void>(buffer, buffer->data())};
        }
        case TypeId::STRING: {
            auto src = reinterpret_cast<const uint32_t*>(slice.data);
            auto buffer = std::make_shared<std::vector<uint32_t>>(indices.size());
            for (size_t i = 0; i < indices.size(); ++i) {
                (*buffer)[i] = src[indices[i]];
            }
            return {buffer->data(), type, indices.size(), std::shared_ptr<void>(buffer, buffer->data())};
        }
        case TypeId::DATE32: {
            auto src = reinterpret_cast<const int32_t*>(slice.data);
            auto buffer = std::make_shared<std::vector<int32_t>>(indices.size());
            for (size_t i = 0; i < indices.size(); ++i) {
                (*buffer)[i] = src[indices[i]];
            }
            return {buffer->data(), type, indices.size(), std::shared_ptr<void>(buffer, buffer->data())};
        }
    }
    throw std::runtime_error("Unknown type");
}

ColumnSlice copy_range(const ColumnSlice& slice,
                       TypeId type,
                       size_t offset,
                       size_t count) {
    switch (type) {
        case TypeId::INT64: {
            auto src = reinterpret_cast<const int64_t*>(slice.data);
            auto buffer = std::make_shared<std::vector<int64_t>>(count);
            std::copy_n(src + offset, count, buffer->begin());
            return {buffer->data(), type, count, std::shared_ptr<void>(buffer, buffer->data())};
        }
        case TypeId::DOUBLE: {
            auto src = reinterpret_cast<const double*>(slice.data);
            auto buffer = std::make_shared<std::vector<double>>(count);
            std::copy_n(src + offset, count, buffer->begin());
            return {buffer->data(), type, count, std::shared_ptr<void>(buffer, buffer->data())};
        }
        case TypeId::STRING: {
            auto src = reinterpret_cast<const uint32_t*>(slice.data);
            auto buffer = std::make_shared<std::vector<uint32_t>>(count);
            std::copy_n(src + offset, count, buffer->begin());
            return {buffer->data(), type, count, std::shared_ptr<void>(buffer, buffer->data())};
        }
        case TypeId::DATE32: {
            auto src = reinterpret_cast<const int32_t*>(slice.data);
            auto buffer = std::make_shared<std::vector<int32_t>>(count);
            std::copy_n(src + offset, count, buffer->begin());
            return {buffer->data(), type, count, std::shared_ptr<void>(buffer, buffer->data())};
        }
    }
    throw std::runtime_error("Unknown type");
}

TypeId infer_type(const Expr* expr, const ExprBindings& bindings) {
    switch (expr->type) {
        case ExprType::COLUMN_REF: {
            auto it = bindings.name_to_index.find(expr->str_val);
            if (it == bindings.name_to_index.end()) {
                throw std::runtime_error("Unknown column: " + expr->str_val);
            }
            return (*bindings.column_types)[it->second];
        }
        case ExprType::LITERAL_INT:
            return TypeId::INT64;
        case ExprType::LITERAL_DOUBLE:
            return TypeId::DOUBLE;
        case ExprType::LITERAL_STRING:
            return TypeId::STRING;
        case ExprType::BINARY_OP: {
            switch (expr->op) {
                case BinaryOp::ADD:
                case BinaryOp::SUB:
                case BinaryOp::MUL:
                case BinaryOp::DIV: {
                    TypeId left = infer_type(expr->left.get(), bindings);
                    TypeId right = infer_type(expr->right.get(), bindings);
                    if (left == TypeId::DOUBLE || right == TypeId::DOUBLE) {
                        return TypeId::DOUBLE;
                    }
                    return TypeId::INT64;
                }
                case BinaryOp::EQ:
                case BinaryOp::NE:
                case BinaryOp::LT:
                case BinaryOp::LE:
                case BinaryOp::GT:
                case BinaryOp::GE:
                case BinaryOp::AND:
                case BinaryOp::OR:
                    return TypeId::INT64;
            }
            break;
        }
        case ExprType::FUNC_CALL:
            throw std::runtime_error("Function call unsupported in projection");
    }
    throw std::runtime_error("Cannot infer expression type");
}

struct ColumnBuilder {
    TypeId type;
    std::shared_ptr<void> storage;
};

ColumnBuilder make_builder(TypeId type) {
    switch (type) {
        case TypeId::INT64:
            return {type, std::make_shared<std::vector<int64_t>>()};
        case TypeId::DOUBLE:
            return {type, std::make_shared<std::vector<double>>()};
        case TypeId::STRING:
            return {type, std::make_shared<std::vector<uint32_t>>()};
        case TypeId::DATE32:
            return {type, std::make_shared<std::vector<int32_t>>()};
    }
    throw std::runtime_error("Unknown column type");
}

void append_value(ColumnBuilder& builder, const ColumnSlice& slice, size_t row) {
    switch (builder.type) {
        case TypeId::INT64: {
            auto vec = std::static_pointer_cast<std::vector<int64_t>>(builder.storage);
            const auto* ptr = reinterpret_cast<const int64_t*>(slice.data);
            vec->push_back(ptr[row]);
            break;
        }
        case TypeId::DOUBLE: {
            auto vec = std::static_pointer_cast<std::vector<double>>(builder.storage);
            const auto* ptr = reinterpret_cast<const double*>(slice.data);
            vec->push_back(ptr[row]);
            break;
        }
        case TypeId::STRING: {
            auto vec = std::static_pointer_cast<std::vector<uint32_t>>(builder.storage);
            const auto* ptr = reinterpret_cast<const uint32_t*>(slice.data);
            vec->push_back(ptr[row]);
            break;
        }
        case TypeId::DATE32: {
            auto vec = std::static_pointer_cast<std::vector<int32_t>>(builder.storage);
            const auto* ptr = reinterpret_cast<const int32_t*>(slice.data);
            vec->push_back(ptr[row]);
            break;
        }
    }
}

void append_value(ColumnBuilder& builder, const Datum& value) {
    switch (builder.type) {
        case TypeId::INT64: {
            auto vec = std::static_pointer_cast<std::vector<int64_t>>(builder.storage);
            if (value.type == TypeId::DOUBLE) {
                vec->push_back(static_cast<int64_t>(value.value.f64_val));
            } else {
                vec->push_back(value.value.i64_val);
            }
            break;
        }
        case TypeId::DOUBLE: {
            auto vec = std::static_pointer_cast<std::vector<double>>(builder.storage);
            if (value.type == TypeId::DOUBLE) {
                vec->push_back(value.value.f64_val);
            } else {
                vec->push_back(static_cast<double>(value.value.i64_val));
            }
            break;
        }
        case TypeId::STRING: {
            auto vec = std::static_pointer_cast<std::vector<uint32_t>>(builder.storage);
            vec->push_back(value.value.str_id);
            break;
        }
        case TypeId::DATE32: {
            auto vec = std::static_pointer_cast<std::vector<int32_t>>(builder.storage);
            if (value.type == TypeId::DATE32) {
                vec->push_back(value.value.date32_val);
            } else {
                vec->push_back(static_cast<int32_t>(value.value.i64_val));
            }
            break;
        }
    }
}

ColumnSlice finalize_builder(const ColumnBuilder& builder) {
    switch (builder.type) {
        case TypeId::INT64: {
            auto vec = std::static_pointer_cast<std::vector<int64_t>>(builder.storage);
            return {vec->data(), builder.type, vec->size(), std::shared_ptr<void>(vec, vec->data())};
        }
        case TypeId::DOUBLE: {
            auto vec = std::static_pointer_cast<std::vector<double>>(builder.storage);
            return {vec->data(), builder.type, vec->size(), std::shared_ptr<void>(vec, vec->data())};
        }
        case TypeId::STRING: {
            auto vec = std::static_pointer_cast<std::vector<uint32_t>>(builder.storage);
            return {vec->data(), builder.type, vec->size(), std::shared_ptr<void>(vec, vec->data())};
        }
        case TypeId::DATE32: {
            auto vec = std::static_pointer_cast<std::vector<int32_t>>(builder.storage);
            return {vec->data(), builder.type, vec->size(), std::shared_ptr<void>(vec, vec->data())};
        }
    }
    throw std::runtime_error("Unknown column type");
}

Datum extract_value(const ColumnSlice& slice, TypeId type, size_t row) {
    switch (type) {
        case TypeId::INT64:
            return Datum::from_i64(reinterpret_cast<const int64_t*>(slice.data)[row]);
        case TypeId::DOUBLE:
            return Datum::from_f64(reinterpret_cast<const double*>(slice.data)[row]);
        case TypeId::STRING:
            return Datum::from_str(reinterpret_cast<const uint32_t*>(slice.data)[row]);
        case TypeId::DATE32:
            return Datum::from_date32(reinterpret_cast<const int32_t*>(slice.data)[row]);
    }
    throw std::runtime_error("Unknown column type");
}

std::vector<Datum> materialize_row(const ExecBatch& batch,
                                   size_t row,
                                   const std::vector<TypeId>& types) {
    std::vector<Datum> values;
    values.reserve(types.size());
    for (size_t i = 0; i < types.size(); ++i) {
        values.push_back(extract_value(batch.columns[i], types[i], row));
    }
    return values;
}

}

ColumnarScan::ColumnarScan(Table* t, std::vector<size_t> idx, size_t batch)
    : table(t), indices(std::move(idx)), offset(0), batch_size(batch) {
    if (!table) {
        throw std::runtime_error("Scan table is null");
    }
    if (indices.empty()) {
        indices.reserve(table->columns.size());
        for (size_t i = 0; i < table->columns.size(); ++i) {
            indices.push_back(i);
        }
    }
    names_.reserve(indices.size());
    types_.reserve(indices.size());
    for (size_t i : indices) {
        names_.push_back(table->columns[i].name);
        types_.push_back(table->columns[i].data->type());
    }
    dict_ = table->dict.get();
}

void ColumnarScan::open() {
    offset = 0;
}

bool ColumnarScan::next(ExecBatch& out) {
    if (indices.empty()) return false;
    size_t row_count = table->columns[indices[0]].data->size();
    if (offset >= row_count) return false;

    size_t take = std::min(batch_size, row_count - offset);
    out.clear();
    out.columns.reserve(indices.size());
    for (size_t idx : indices) {
        auto& col = table->columns[idx];
        auto type = col.data->type();
        const void* ptr = nullptr;
        switch (type) {
            case TypeId::INT64: {
                auto* vec = dynamic_cast<ColumnVector<int64_t>*>(col.data.get());
                ptr = vec->data.data() + offset;
                break;
            }
            case TypeId::DOUBLE: {
                auto* vec = dynamic_cast<ColumnVector<double>*>(col.data.get());
                ptr = vec->data.data() + offset;
                break;
            }
            case TypeId::STRING: {
                auto* vec = dynamic_cast<ColumnVector<uint32_t>*>(col.data.get());
                ptr = vec->data.data() + offset;
                break;
            }
            case TypeId::DATE32: {
                auto* vec = dynamic_cast<ColumnVector<int32_t>*>(col.data.get());
                ptr = vec->data.data() + offset;
                break;
            }
        }
        out.columns.push_back({ptr, type, take, {}});
    }
    out.length = take;
    offset += take;
    return true;
}

void ColumnarScan::close() {}

Selection::Selection(std::unique_ptr<Operator> c, std::unique_ptr<Expr> pred)
    : child(std::move(c)), predicate(std::move(pred)) {
    if (!child) {
        throw std::runtime_error("Selection child is null");
    }
    names_ = child->output_names();
    types_ = child->output_types();
    dict_ = child->dictionary();
    bindings = make_bindings(names_, types_, dict_);
}

void Selection::open() {
    child->open();
}

bool Selection::next(ExecBatch& out) {
    ExecBatch in;
    while (child->next(in)) {
        if (!predicate) {
            out = in;
            return true;
        }
        std::vector<size_t> selected;
        selected.reserve(in.length);
        for (size_t row = 0; row < in.length; ++row) {
            if (evaluate_predicate(predicate.get(), in, row, bindings)) {
                selected.push_back(row);
            }
        }
        if (selected.empty()) {
            continue;
        }
        out.clear();
        out.columns.reserve(in.columns.size());
        for (size_t col = 0; col < in.columns.size(); ++col) {
            out.columns.push_back(copy_selected(in.columns[col], types_[col], selected));
        }
        out.length = selected.size();
        return true;
    }
    return false;
}

void Selection::close() {
    child->close();
}

Project::Project(std::unique_ptr<Operator> c,
                 std::vector<std::unique_ptr<Expr>> exprs,
                 std::vector<std::string> alias_list)
    : child(std::move(c)), expressions(std::move(exprs)), aliases(std::move(alias_list)) {
    if (!child) {
        throw std::runtime_error("Project child is null");
    }
    input_names = child->output_names();
    input_types = child->output_types();
    dict_ = child->dictionary();
    bindings = make_bindings(input_names, input_types, dict_);

    names_.clear();
    types_.clear();
    names_.reserve(expressions.size());
    types_.reserve(expressions.size());
    for (size_t i = 0; i < expressions.size(); ++i) {
        types_.push_back(infer_type(expressions[i].get(), bindings));
        if (i < aliases.size() && !aliases[i].empty()) {
            names_.push_back(aliases[i]);
        } else if (expressions[i]->type == ExprType::COLUMN_REF) {
            names_.push_back(expressions[i]->str_val);
        } else {
            names_.push_back("expr");
        }
    }
}

void Project::open() {
    child->open();
}

bool Project::next(ExecBatch& out) {
    ExecBatch in;
    if (!child->next(in)) {
        return false;
    }
    out.clear();
    out.columns.reserve(expressions.size());
    for (size_t i = 0; i < expressions.size(); ++i) {
        auto type = types_[i];
        if (expressions[i]->type == ExprType::COLUMN_REF) {
            auto it = bindings.name_to_index.find(expressions[i]->str_val);
            if (it == bindings.name_to_index.end()) {
                throw std::runtime_error("Unknown column: " + expressions[i]->str_val);
            }
            out.columns.push_back(in.columns[it->second]);
            continue;
        }
        switch (type) {
            case TypeId::INT64: {
                auto buffer = std::make_shared<std::vector<int64_t>>(in.length);
                for (size_t row = 0; row < in.length; ++row) {
                    auto value = evaluate_expr(expressions[i].get(), in, row, bindings);
                    (*buffer)[row] = value.type == TypeId::INT64 ? value.value.i64_val
                                                                  : static_cast<int64_t>(value.value.f64_val);
                }
                out.columns.push_back({buffer->data(), type, in.length, std::shared_ptr<void>(buffer, buffer->data())});
                break;
            }
            case TypeId::DOUBLE: {
                auto buffer = std::make_shared<std::vector<double>>(in.length);
                for (size_t row = 0; row < in.length; ++row) {
                    auto value = evaluate_expr(expressions[i].get(), in, row, bindings);
                    (*buffer)[row] = value.type == TypeId::DOUBLE ? value.value.f64_val
                                                                  : static_cast<double>(value.value.i64_val);
                }
                out.columns.push_back({buffer->data(), type, in.length, std::shared_ptr<void>(buffer, buffer->data())});
                break;
            }
            case TypeId::DATE32: {
                auto buffer = std::make_shared<std::vector<int32_t>>(in.length);
                for (size_t row = 0; row < in.length; ++row) {
                    auto value = evaluate_expr(expressions[i].get(), in, row, bindings);
                    (*buffer)[row] = value.value.date32_val;
                }
                out.columns.push_back({buffer->data(), type, in.length, std::shared_ptr<void>(buffer, buffer->data())});
                break;
            }
            case TypeId::STRING: {
                auto buffer = std::make_shared<std::vector<uint32_t>>(in.length);
                for (size_t row = 0; row < in.length; ++row) {
                    auto value = evaluate_expr(expressions[i].get(), in, row, bindings);
                    (*buffer)[row] = value.value.str_id;
                }
                out.columns.push_back({buffer->data(), type, in.length, std::shared_ptr<void>(buffer, buffer->data())});
                break;
            }
        }
    }
    out.length = in.length;
    return true;
}

void Project::close() {
    child->close();
}

Limit::Limit(std::unique_ptr<Operator> c, int64_t n)
    : child(std::move(c)), limit(n), produced(0) {
    if (!child) {
        throw std::runtime_error("Limit child is null");
    }
    names_ = child->output_names();
    types_ = child->output_types();
    dict_ = child->dictionary();
}

void Limit::open() {
    child->open();
    produced = 0;
    cache_valid = false;
    cache_offset = 0;
    cache.clear();
}

bool Limit::next(ExecBatch& out) {
    if (produced >= limit) {
        return false;
    }
    while (produced < limit) {
        if (!cache_valid) {
            cache.clear();
            if (!child->next(cache)) {
                return false;
            }
            cache_offset = 0;
            cache_valid = true;
        }
        size_t available = cache.length - cache_offset;
        size_t remaining = static_cast<size_t>(limit - produced);
        size_t take = std::min(available, remaining);
        if (take == 0) {
            cache_valid = false;
            continue;
        }
        out.clear();
        out.columns.reserve(cache.columns.size());
        for (size_t i = 0; i < cache.columns.size(); ++i) {
            out.columns.push_back(copy_range(cache.columns[i], types_[i], cache_offset, take));
        }
        out.length = take;
        produced += take;
        cache_offset += take;
        if (cache_offset >= cache.length) {
            cache_valid = false;
        }
        return true;
    }
    return false;
}

void Limit::close() {
    child->close();
    cache.clear();
    cache_valid = false;
    cache_offset = 0;
}

size_t HashJoin::KeyHash::operator()(const Key& key) const {
    size_t seed = 0;
    for (const auto& value : key.values) {
        size_t h = 0;
        switch (value.type) {
            case TypeId::INT64:
                h = std::hash<int64_t>{}(value.value.i64_val);
                break;
            case TypeId::DOUBLE:
                h = std::hash<double>{}(value.value.f64_val);
                break;
            case TypeId::STRING:
                h = std::hash<uint32_t>{}(value.value.str_id);
                break;
            case TypeId::DATE32:
                h = std::hash<int32_t>{}(value.value.date32_val);
                break;
        }
        seed ^= h + 0x9e3779b97f4a7c15ULL + (seed << 6) + (seed >> 2);
    }
    return seed;
}

bool HashJoin::KeyEqual::operator()(const Key& lhs, const Key& rhs) const {
    if (lhs.values.size() != rhs.values.size()) {
        return false;
    }
    for (size_t i = 0; i < lhs.values.size(); ++i) {
        const Datum& a = lhs.values[i];
        const Datum& b = rhs.values[i];
        if (a.type != b.type) return false;
        switch (a.type) {
            case TypeId::INT64:
                if (a.value.i64_val != b.value.i64_val) return false;
                break;
            case TypeId::DOUBLE:
                if (a.value.f64_val != b.value.f64_val) return false;
                break;
            case TypeId::STRING:
                if (a.value.str_id != b.value.str_id) return false;
                break;
            case TypeId::DATE32:
                if (a.value.date32_val != b.value.date32_val) return false;
                break;
        }
    }
    return true;
}

HashJoin::HashJoin(std::unique_ptr<Operator> left,
                   std::unique_ptr<Operator> right,
                   std::vector<std::string> left_keys,
                   std::vector<std::string> right_keys,
                   std::unique_ptr<Expr> residual)
    : left_child(std::move(left)),
      right_child(std::move(right)),
      left_key_names(std::move(left_keys)),
      right_key_names(std::move(right_keys)),
      residual_filter(std::move(residual)) {
    if (!left_child || !right_child) {
        throw std::runtime_error("Join operands cannot be null");
    }
    left_names = left_child->output_names();
    left_types = left_child->output_types();
    right_names = right_child->output_names();
    right_types = right_child->output_types();

    names_ = left_names;
    names_.insert(names_.end(), right_names.begin(), right_names.end());
    types_ = left_types;
    types_.insert(types_.end(), right_types.begin(), right_types.end());

    Dictionary* left_dict = left_child->dictionary();
    Dictionary* right_dict = right_child->dictionary();
    bool left_has_string = std::any_of(left_types.begin(), left_types.end(), [](TypeId t) { return t == TypeId::STRING; });
    bool right_has_string = std::any_of(right_types.begin(), right_types.end(), [](TypeId t) { return t == TypeId::STRING; });
    if (left_has_string && left_dict) {
        dict_ = left_dict;
    } else if (right_has_string && right_dict) {
        dict_ = right_dict;
    } else {
        dict_ = left_dict ? left_dict : right_dict;
    }

    left_bindings = make_bindings(left_names, left_types, left_child->dictionary());
    right_bindings = make_bindings(right_names, right_types, right_child->dictionary());

    auto resolve = [](const std::vector<std::string>& keys,
                      const std::vector<std::string>& columns) {
        std::vector<size_t> indices;
        indices.reserve(keys.size());
        for (const auto& key : keys) {
            auto it = std::find(columns.begin(), columns.end(), key);
            if (it == columns.end()) {
                throw std::runtime_error("Join key not found: " + key);
            }
            indices.push_back(static_cast<size_t>(std::distance(columns.begin(), it)));
        }
        return indices;
    };

    left_key_indices = resolve(left_key_names, left_names);
    right_key_indices = resolve(right_key_names, right_names);
    if (left_key_indices.size() != right_key_indices.size()) {
        throw std::runtime_error("Join key cardinality mismatch");
    }

    left_key_types.reserve(left_key_indices.size());
    right_key_types.reserve(right_key_indices.size());
    for (size_t index : left_key_indices) {
        left_key_types.push_back(left_types[index]);
    }
    for (size_t index : right_key_indices) {
        right_key_types.push_back(right_types[index]);
    }
}

void HashJoin::open() {
    hash_table.clear();
    build_rows.clear();
    probe_batch.clear();
    probe_batch_valid = false;
    probe_row_index = 0;
    current_matches.clear();
    match_index = 0;

    right_child->open();
    ExecBatch build_batch;
    size_t row_id = 0;
    while (right_child->next(build_batch)) {
        for (size_t row = 0; row < build_batch.length; ++row) {
            Key key = build_key(build_batch, row, right_key_indices, right_key_types);
            build_rows.push_back(materialize_row(build_batch, row, right_types));
            hash_table[key].push_back(row_id);
            ++row_id;
        }
    }
    right_child->close();

    left_child->open();
}

bool HashJoin::next(ExecBatch& out) {
    constexpr size_t batch_target = 4096;
    std::vector<ColumnBuilder> builders;
    builders.reserve(types_.size());
    for (auto type : types_) {
        builders.push_back(make_builder(type));
    }

    size_t produced = 0;
    while (produced < batch_target) {
        if (match_index >= current_matches.size()) {
            bool found = false;
            while (!found) {
                if (!probe_batch_valid || probe_row_index >= probe_batch.length) {
                    probe_batch_valid = left_child->next(probe_batch);
                    if (!probe_batch_valid) {
                        break;
                    }
                    probe_row_index = 0;
                }
                if (!probe_batch_valid) {
                    break;
                }
                Key key = build_key(probe_batch, probe_row_index, left_key_indices, left_key_types);
                auto it = hash_table.find(key);
                if (it == hash_table.end()) {
                    ++probe_row_index;
                    continue;
                }
                current_matches = it->second;
                match_index = 0;
                found = true;
            }
            if (!found) {
                break;
            }
        }

        while (match_index < current_matches.size() && produced < batch_target) {
            size_t right_index = current_matches[match_index];
            const auto& right_row = build_rows[right_index];
            size_t builder_idx = 0;
            for (size_t col = 0; col < left_types.size(); ++col) {
                append_value(builders[builder_idx], probe_batch.columns[col], probe_row_index);
                ++builder_idx;
            }
            for (size_t col = 0; col < right_row.size(); ++col) {
                append_value(builders[builder_idx], right_row[col]);
                ++builder_idx;
            }
            ++produced;
            ++match_index;
        }

        if (match_index >= current_matches.size()) {
            current_matches.clear();
            match_index = 0;
            ++probe_row_index;
        }
    }

    if (produced == 0) {
        out.clear();
        return false;
    }

    out.clear();
    out.columns.reserve(types_.size());
    for (auto& builder : builders) {
        out.columns.push_back(finalize_builder(builder));
    }
    out.length = produced;
    return true;
}

void HashJoin::close() {
    left_child->close();
    probe_batch.clear();
    probe_batch_valid = false;
    current_matches.clear();
    match_index = 0;
}

HashJoin::Key HashJoin::build_key(const ExecBatch& batch,
                                  size_t row,
                                  const std::vector<size_t>& indices,
                                  const std::vector<TypeId>& key_types) const {
    Key key;
    key.values.reserve(indices.size());
    for (size_t i = 0; i < indices.size(); ++i) {
        size_t column = indices[i];
        key.values.push_back(extract_value(batch.columns[column], key_types[i], row));
    }
    return key;
}

}
