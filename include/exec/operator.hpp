#pragma once

#include <vector>
#include <memory>
#include <iostream>
#include <unordered_map>
#include "types.h"
#include "exec/execution_types.hpp"
#include "exec/expression.h"
#include "exec/formatter.hpp"
#include "storage/table.h"
#include "parser/ast.h"

namespace bosql {

// Physical operator interface
struct Operator {
    virtual ~Operator() = default;
    virtual void open() = 0;
    virtual bool next(ExecBatch& out) = 0;
    virtual void close() = 0;

    const std::vector<std::string>& output_names() const { return names_; }
    const std::vector<TypeId>& output_types() const { return types_; }
    Dictionary* dictionary() const { return dict_; }

protected:
    std::vector<std::string> names_;
    std::vector<TypeId> types_;
    Dictionary* dict_ = nullptr;
};

struct ColumnarScan : public Operator {
    ColumnarScan(Table* t, std::vector<size_t> idx, size_t batch = 4096);

    void open() override;
    bool next(ExecBatch& out) override;
    void close() override;

private:
    Table* table;
    std::vector<size_t> indices;
    size_t offset;
    size_t batch_size;
};

struct Selection : public Operator {
    Selection(std::unique_ptr<Operator> c, std::unique_ptr<Expr> pred);

    void open() override;
    bool next(ExecBatch& out) override;
    void close() override;

private:
    std::unique_ptr<Operator> child;
    std::unique_ptr<Expr> predicate;
    ExprBindings bindings;
};

struct Project : public Operator {
    Project(std::unique_ptr<Operator> c,
            std::vector<std::unique_ptr<Expr>> exprs,
            std::vector<std::string> aliases);

    void open() override;
    bool next(ExecBatch& out) override;
    void close() override;

private:
    std::unique_ptr<Operator> child;
    std::vector<std::unique_ptr<Expr>> expressions;
    std::vector<std::string> aliases;
    ExprBindings bindings;
    std::vector<std::string> input_names;
    std::vector<TypeId> input_types;
    std::vector<int> direct_indices;
};

struct HashJoin : public Operator {
    HashJoin(std::unique_ptr<Operator> left,
             std::unique_ptr<Operator> right,
             std::vector<std::string> left_keys,
             std::vector<std::string> right_keys,
             std::unique_ptr<Expr> residual);

    void open() override;
    bool next(ExecBatch& out) override;
    void close() override;

private:
    std::unique_ptr<Operator> left_child;
    std::unique_ptr<Operator> right_child;
    std::vector<std::string> left_key_names;
    std::vector<std::string> right_key_names;
    std::unique_ptr<Expr> residual_filter;
    std::vector<size_t> left_key_indices;
    std::vector<size_t> right_key_indices;
    std::vector<TypeId> left_key_types;
    std::vector<TypeId> right_key_types;
    std::vector<std::string> left_names;
    std::vector<TypeId> left_types;
    std::vector<std::string> right_names;
    std::vector<TypeId> right_types;
    ExprBindings left_bindings;
    ExprBindings right_bindings;

    struct Key {
        std::vector<Datum> values;
    };

    struct KeyHash {
        size_t operator()(const Key& key) const;
    };

    struct KeyEqual {
        bool operator()(const Key& lhs, const Key& rhs) const;
    };

    std::unordered_map<Key, std::vector<size_t>, KeyHash, KeyEqual> hash_table;
    std::vector<std::vector<Datum>> build_rows;
    ExecBatch probe_batch;
    bool probe_batch_valid = false;
    size_t probe_row_index = 0;
    std::vector<size_t> current_matches;
    size_t match_index = 0;

    Key build_key(const ExecBatch& batch,
                  size_t row,
                  const std::vector<size_t>& indices,
                  const std::vector<TypeId>& key_types) const;
};

struct AggregateSpec {
    std::string func_name;
    std::unique_ptr<Expr> arg;
    std::string alias;
};

struct HashAggregate : public Operator {
    HashAggregate(std::unique_ptr<Operator> child,
                  std::vector<std::unique_ptr<Expr>> group_exprs,
                  std::vector<AggregateSpec> aggregates);

    void open() override;
    bool next(ExecBatch& out) override;
    void close() override;

private:
    struct AggState {
        double sum = 0.0;
        int64_t count = 0;
    };

    std::unique_ptr<Operator> child;
    std::vector<std::unique_ptr<Expr>> group_exprs;
    std::vector<AggregateSpec> aggregates;
    ExprBindings child_bindings;

    struct GroupKeyHash {
        size_t operator()(const std::vector<Datum>& key) const;
    };

    struct GroupKeyEqual {
        bool operator()(const std::vector<Datum>& lhs, const std::vector<Datum>& rhs) const;
    };

    std::unordered_map<std::vector<Datum>, std::vector<AggState>, GroupKeyHash, GroupKeyEqual> groups;
    std::vector<TypeId> group_types;
    std::vector<TypeId> agg_types;
    bool results_ready = false;
    bool child_consumed = false;
    size_t emit_index = 0;
    std::vector<std::vector<Datum>> result_keys;
    std::vector<std::vector<AggState>> result_aggs;
};

struct OrderBy : public Operator {
    struct SortKey {
        std::unique_ptr<Expr> expr;
        bool asc;
    };

    OrderBy(std::unique_ptr<Operator> child,
            std::vector<SortKey> sort_keys);

    void open() override;
    bool next(ExecBatch& out) override;
    void close() override;

private:
    std::unique_ptr<Operator> child;
    std::vector<SortKey> sort_keys;
    ExprBindings bindings;
    struct SortedRow {
        std::vector<Datum> values;
        std::vector<Datum> sort_values;
    };
    std::vector<SortedRow> rows;
    size_t emit_index = 0;
    bool materialized = false;
    bool child_consumed = false;
};

struct Limit : public Operator {
    Limit(std::unique_ptr<Operator> c, int64_t n);

    void open() override;
    bool next(ExecBatch& out) override;
    void close() override;

private:
    std::unique_ptr<Operator> child;
    int64_t limit;
    int64_t produced;
    ExecBatch cache;
    size_t cache_offset = 0;
    bool cache_valid = false;
};

// Execution driver
void run_query(std::unique_ptr<Operator> root,
               const std::vector<std::string>& col_names,
               const std::vector<TypeId>& col_types,
               Formatter& formatter,
               const Dictionary* dict = nullptr);

}
