#pragma once

#include <vector>
#include <memory>
#include <iostream>
#include <span>
#include <functional>
#include "types.h"
#include "storage/table.h"
#include "parser/ast.h"

namespace bosql {

// Column slice with optional cleanup
struct ColumnSlice {
    const void* data;
    TypeId type;
    size_t length;
    std::function<void()> cleanup;
};

// ExecBatch for execution: type-erased column slices
struct ExecBatch {
    std::vector<ColumnSlice> columns;
    size_t length;
};

// Typed accessor helper
template<typename T>
std::span<const T> get_col(const ExecBatch& batch, size_t i) {
    if (batch.columns[i].type != type_id_for<T>()) {
        throw std::runtime_error("Type mismatch");
    }
    return {reinterpret_cast<const T*>(batch.columns[i].data), batch.columns[i].length};
}

// Physical operator interface
struct Operator {
    virtual ~Operator() = default;
    virtual void open() = 0;
    virtual bool next(ExecBatch& out) = 0;
    virtual void close() = 0;
};

// ColumnarScan: scans a table in batches
struct ColumnarScan : public Operator {
    Table* table;
    size_t offset = 0;
    size_t batch_size;

    ColumnarScan(Table* t, size_t batch = 4096) : table(t), batch_size(batch) {}

    void open() override { offset = 0; }

    bool next(ExecBatch& out) override {
        size_t row_count = table->columns.empty() ? 0 : table->columns[0].data->size();
        if (offset >= row_count) return false;

        size_t take = std::min(batch_size, row_count - offset);
        out.columns.clear();
        for (auto& col : table->columns) {
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

    void close() override {}
};

// Selection: filters rows based on predicate (hardcoded for MVP)
struct Selection : public Operator {
    std::unique_ptr<Operator> child;
    // For MVP: hardcoded filter on first column > 10
    int filter_col = 0;
    int64_t filter_value = 10;

    Selection(std::unique_ptr<Operator> c) : child(std::move(c)) {}

    void open() override { child->open(); }

    bool next(ExecBatch& out) override {
        ExecBatch in;
        while (child->next(in)) {
            // Assume filter on INT64 column 0
            if (in.columns[filter_col].type != TypeId::INT64) continue;
            const int64_t* col_data = reinterpret_cast<const int64_t*>(in.columns[filter_col].data);
            std::vector<size_t> selected_indices;
            for (size_t i = 0; i < in.length; ++i) {
                if (col_data[i] > filter_value) {
                    selected_indices.push_back(i);
                }
            }
            if (selected_indices.empty()) continue;

            // Build output batch with selected rows
            out.columns.resize(in.columns.size());
            out.length = selected_indices.size();
            for (size_t c = 0; c < in.columns.size(); ++c) {
                auto type = in.columns[c].type;
                switch (type) {
                    case TypeId::INT64: {
                        std::vector<int64_t> new_col_vec(out.length);
                        const int64_t* src = reinterpret_cast<const int64_t*>(in.columns[c].data);
                        for (size_t i = 0; i < out.length; ++i) {
                            new_col_vec[i] = src[selected_indices[i]];
                        }
                        out.columns[c] = {new_col_vec.data(), type, out.length, [vec=std::move(new_col_vec)](){}};
                        break;
                    }
                    // Add other types as needed
                    default:
                        // For MVP, skip
                        break;
                }
            }
            return true;
        }
        return false;
    }

    void close() override { child->close(); }
};

// Project: selects specific columns
struct Project : public Operator {
    std::unique_ptr<Operator> child;
    std::vector<int> col_indices;

    Project(std::unique_ptr<Operator> c, std::vector<int> idx)
        : child(std::move(c)), col_indices(std::move(idx)) {}

    void open() override { child->open(); }

    bool next(ExecBatch& out) override {
        ExecBatch in;
        if (!child->next(in)) return false;

        out.columns.clear();
        for (int idx : col_indices) {
            out.columns.push_back(in.columns[idx]);
        }
        out.length = in.length;
        return true;
    }

    void close() override { child->close(); }
};

// Limit: restricts number of rows
struct Limit : public Operator {
    std::unique_ptr<Operator> child;
    int64_t limit;
    int64_t produced = 0;

    Limit(std::unique_ptr<Operator> c, int64_t n)
        : child(std::move(c)), limit(n) {}

    void open() override { child->open(); }

    bool next(ExecBatch& out) override {
        if (produced >= limit) return false;

        ExecBatch in;
        if (!child->next(in)) return false;

        size_t take = std::min<int64_t>(limit - produced, in.length);
        out = in; // shallow copy
        out.length = take;
        produced += take;
        return true;
    }

    void close() override { child->close(); }
};

// Execution driver
void run_query(std::unique_ptr<Operator> root, const std::vector<std::string>& col_names, const std::vector<TypeId>& col_types, const Dictionary* dict = nullptr);

} // namespace bosql