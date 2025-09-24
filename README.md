# bo-sql

A read-optimized, in-memory, single-threaded SQL engine in C++.

## Goal

Build a read-optimized, in-memory, single-threaded SQL engine that:

- Loads a few CSV tables into a columnar store (no updates after load).
- Supports a minimal SQL subset: SELECT … FROM … WHERE …, INNER JOIN (equi-join), GROUP BY (SUM, COUNT, AVG), ORDER BY, LIMIT, and basic expressions.
- Types: INT64, DOUBLE, STRING (dictionary-encoded), DATE (as int32 yyyymmdd).
- Deterministic execution with a small cost-based heuristic (or rule-based) optimizer.
- Vectorized execution (batch size configurable, default 4096 rows).
- CLI REPL: load tables, run SQL, pretty-print results, EXPLAIN.

Non-goals: No transactions, WAL, MVCC, indexes, deletes/updates, subqueries, window functions, outer joins, UDFs, spill-to-disk.

## Dependencies

- C++20 compiler (e.g., clang or gcc)
- Meson build system
- Ninja build tool
- Catch2 (included as subproject)

## Getting Started

### Building

1. Install dependencies:
   - Meson: `pip install meson`
   - Ninja: `brew install ninja` (macOS) or `apt install ninja-build` (Ubuntu)

2. Clone the repository and build:
   ```bash
   git clone <repo-url>
   cd bo-sql
   meson setup build
   ninja -C build
   ```

### Running

After building, run the CLI REPL:
```bash
./build/cli/bo-sql
```

Commands:
- `LOAD TABLE name FROM 'file.csv' WITH (delimiter=',', header=true);`
- `SHOW TABLES;`
- `DESCRIBE table_name;`
- `EXPLAIN SELECT ...;`
- `SELECT ...;`

## Development Milestones

### M0 – Skeleton (½–1 day)
- Meson/CMake project; engine/, parser/, planner/, exec/, storage/, catalog/, cli/, tests/.
- Common types (i64, f64, StrId, Date32) and ColumnVector<T>.

### M1 – Storage + Catalog (1–2 days)
- LOAD CSV into columnar, dictionary-encoded table.
- Catalog tracks tables, columns, types, stats (min/max, NDV estimate, row count).

### M2 – Parser + AST (1–2 days)
- Handwritten recursive-descent for the subset.
- AST nodes for SelectStmt, TableRef, Join, Expr.

### M3 – Logical algebra IR (1 day)
- Nodes: LogicalScan, LogicalFilter, LogicalProject, LogicalHashJoin, LogicalAggregate, LogicalOrder, LogicalLimit.

### M4 – Optimizer (2–3 days)
- Rule-based: Predicate pushdown, projection pruning, join reordering.

### M5 – Physical plan + Vectorized exec (3–4 days)
- Vectorized Volcano operators: Scan, Selection, Project, HashJoin, HashAggregate, OrderBy, Limit.

### M6 – REPL + EXPLAIN + Tests (2–3 days)
- CLI REPL, EXPLAIN, unit tests.

## Sample Queries

Q1: Revenue by day
```sql
SELECT order_date, SUM(total) AS rev
FROM orders
WHERE status = 'COMPLETE' AND order_date BETWEEN 20240101 AND 20240131
GROUP BY order_date
ORDER BY order_date;
```

Q2: Top SKUs by revenue
```sql
SELECT l.sku, SUM(l.qty * l.price) AS rev
FROM lineitem l JOIN orders o ON l.order_id = o.order_id
WHERE o.status = 'COMPLETE'
GROUP BY l.sku
ORDER BY rev DESC
LIMIT 20;
```


## Design Choices

- Columnar + vectorized: shows memory bandwidth, CPU cache, SIMDable loops, simple predicate pushdown.
- Dictionary encoding for strings: design ID vs payload columns and equality predicate acceleration.
- Rule-based optimizer: implement real rewrites and simple stats; enough to compare plans.
- Hash join + hash agg only: one great path > many mediocre ones.
- In-memory only: ignore buffer manager & logging; still real for analytics