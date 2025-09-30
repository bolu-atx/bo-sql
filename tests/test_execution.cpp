#include <algorithm>
#include <catch2/catch_all.hpp>
#include "catalog/catalog.h"
#include "exec/operator.hpp"
#include "exec/physical_planner.h"
#include "logical/planner.h"
#include "parser/parser.h"

using namespace bosql;

namespace {

Table make_orders_table() {
    Table table;
    table.dict = std::make_shared<Dictionary>();

    auto id_col = std::make_unique<ColumnVector<int64_t>>();
    id_col->append(1);
    id_col->append(2);
    id_col->append(3);

    auto qty_col = std::make_unique<ColumnVector<int64_t>>();
    qty_col->append(10);
    qty_col->append(20);
    qty_col->append(30);

    table.columns.push_back({"orders.id", std::move(id_col)});
    table.columns.push_back({"orders.qty", std::move(qty_col)});
    return table;
}

TableMeta make_orders_meta() {
    std::vector<ColumnMeta> cols;
    cols.emplace_back("orders.id", TypeId::INT64);
    cols.emplace_back("orders.qty", TypeId::INT64);
    return TableMeta("orders", std::move(cols), 3);
}

Table make_detail_table(std::shared_ptr<Dictionary>& dict) {
    Table table;
    table.dict = dict;

    auto id_col = std::make_unique<ColumnVector<int64_t>>();
    id_col->append(1);
    id_col->append(2);
    id_col->append(4);

    auto region_col = std::make_unique<ColumnVector<uint32_t>>();
    region_col->append(table.dict->get_or_add("north"));
    region_col->append(table.dict->get_or_add("south"));
    region_col->append(table.dict->get_or_add("west"));

    table.columns.push_back({"detail.id", std::move(id_col)});
    table.columns.push_back({"detail.region", std::move(region_col)});
    return table;
}

TableMeta make_detail_meta() {
    std::vector<ColumnMeta> cols;
    cols.emplace_back("detail.id", TypeId::INT64);
    cols.emplace_back("detail.region", TypeId::STRING);
    return TableMeta("detail", std::move(cols), 3);
}

std::vector<std::vector<std::string>> execute_plan(std::unique_ptr<Operator> root, Dictionary* dict) {
    std::vector<std::vector<std::string>> rows;
    root->open();
    ExecBatch batch;
    while (root->next(batch)) {
        for (size_t row = 0; row < batch.length; ++row) {
            std::vector<std::string> out_row;
            out_row.reserve(batch.columns.size());
            for (size_t col = 0; col < batch.columns.size(); ++col) {
                switch (batch.columns[col].type) {
                    case TypeId::INT64: {
                        auto values = get_col<int64_t>(batch, col);
                        out_row.push_back(std::to_string(values[row]));
                        break;
                    }
                    case TypeId::DOUBLE: {
                        auto values = get_col<double>(batch, col);
                        out_row.push_back(std::to_string(values[row]));
                        break;
                    }
                    case TypeId::STRING: {
                        auto values = get_col<uint32_t>(batch, col);
                        if (dict) {
                            out_row.push_back(dict->get(values[row]));
                        } else {
                            out_row.push_back(std::to_string(values[row]));
                        }
                        break;
                    }
                    case TypeId::DATE32: {
                        auto values = get_col<int32_t>(batch, col);
                        out_row.push_back(std::to_string(values[row]));
                        break;
                    }
                }
            }
            rows.push_back(std::move(out_row));
        }
    }
    root->close();
    return rows;
}

Catalog build_orders_catalog() {
    Catalog catalog;
    Table orders = make_orders_table();
    TableMeta orders_meta = make_orders_meta();
    catalog.register_table(std::move(orders), std::move(orders_meta));
    return catalog;
}

Catalog build_full_catalog(std::shared_ptr<Dictionary>& detail_dict) {
    Catalog catalog = build_orders_catalog();
    detail_dict = std::make_shared<Dictionary>();
    Table detail = make_detail_table(detail_dict);
    TableMeta detail_meta = make_detail_meta();
    catalog.register_table(std::move(detail), std::move(detail_meta));
    return catalog;
}

} // namespace

TEST_CASE("Selection filters rows", "[exec]") {
    Catalog catalog = build_orders_catalog();
    SelectStmt stmt = parse_sql("SELECT orders.id FROM orders WHERE orders.qty > 15");
    LogicalPlanner planner;
    auto logical = planner.build_logical_plan(stmt);
    auto physical = build_physical_plan(logical.get(), catalog);

    auto rows = execute_plan(std::move(physical), nullptr);
    REQUIRE(rows.size() == 2);
    REQUIRE(rows[0][0] == "2");
    REQUIRE(rows[1][0] == "3");
}

TEST_CASE("Projection evaluates expressions", "[exec]") {
    Catalog catalog = build_orders_catalog();
    SelectStmt stmt = parse_sql("SELECT orders.id, orders.qty * 2 AS double_qty FROM orders");
    LogicalPlanner planner;
    auto logical = planner.build_logical_plan(stmt);
    auto physical = build_physical_plan(logical.get(), catalog);

    auto rows = execute_plan(std::move(physical), nullptr);
    REQUIRE(rows.size() == 3);
    REQUIRE(rows[0][0] == "1");
    REQUIRE(rows[0][1] == "20");
    REQUIRE(rows[2][0] == "3");
    REQUIRE(rows[2][1] == "60");
}

TEST_CASE("Limit short-circuits output", "[exec]") {
    Catalog catalog = build_orders_catalog();
    SelectStmt stmt = parse_sql("SELECT orders.id FROM orders LIMIT 2");
    LogicalPlanner planner;
    auto logical = planner.build_logical_plan(stmt);
    auto physical = build_physical_plan(logical.get(), catalog);

    auto rows = execute_plan(std::move(physical), nullptr);
    REQUIRE(rows.size() == 2);
    REQUIRE(rows[0][0] == "1");
    REQUIRE(rows[1][0] == "2");
}

TEST_CASE("Hash join produces matching rows", "[exec]") {
    std::shared_ptr<Dictionary> detail_dict;
    Catalog catalog = build_full_catalog(detail_dict);

    SelectStmt stmt = parse_sql("SELECT orders.id, detail.region FROM orders INNER JOIN detail ON orders.id = detail.id");
    LogicalPlanner planner;
    auto logical = planner.build_logical_plan(stmt);
    auto physical = build_physical_plan(logical.get(), catalog);

    Dictionary* dict = physical->dictionary();
    REQUIRE(dict != nullptr);
    auto rows = execute_plan(std::move(physical), dict);
    REQUIRE(rows.size() == 2);
    REQUIRE(rows[0][0] == "1");
    REQUIRE(rows[0][1] == "north");
    REQUIRE(rows[1][0] == "2");
    REQUIRE(rows[1][1] == "south");
}

TEST_CASE("Aggregate computes totals", "[exec]") {
    std::shared_ptr<Dictionary> detail_dict;
    Catalog catalog = build_full_catalog(detail_dict);
    SelectStmt stmt = parse_sql("SELECT detail.region, SUM(orders.qty) AS total FROM orders INNER JOIN detail ON orders.id = detail.id GROUP BY detail.region");
    LogicalPlanner planner;
    auto logical = planner.build_logical_plan(stmt);
    auto physical = build_physical_plan(logical.get(), catalog);

    const auto& out_names = physical->output_names();
    REQUIRE(out_names.size() == 2);
    REQUIRE(out_names[0] == "detail.region");
    REQUIRE(out_names[1] == "total");

    Dictionary* dict = physical->dictionary();
    auto rows = execute_plan(std::move(physical), dict);
    REQUIRE(rows.size() == 2);
    std::sort(rows.begin(), rows.end());
    REQUIRE(rows[0][0] == "north");
    REQUIRE(rows[0][1] == "10");
    REQUIRE(rows[1][0] == "south");
    REQUIRE(rows[1][1] == "20");
}

TEST_CASE("Global aggregate counts rows", "[exec]") {
    Catalog catalog = build_orders_catalog();
    SelectStmt stmt = parse_sql("SELECT COUNT(*) FROM orders");
    LogicalPlanner planner;
    auto logical = planner.build_logical_plan(stmt);
    auto physical = build_physical_plan(logical.get(), catalog);
    REQUIRE(dynamic_cast<HashAggregate*>(physical.get()) != nullptr);

    const auto& count_names = physical->output_names();
    REQUIRE(count_names.size() == 1);
    REQUIRE(count_names[0] == "COUNT(*)");

    auto rows = execute_plan(std::move(physical), nullptr);
    REQUIRE(rows.size() == 1);
    REQUIRE(rows[0][0] == "3");
}

TEST_CASE("Order by sorts descending", "[exec]") {
    Catalog catalog = build_orders_catalog();
    SelectStmt stmt = parse_sql("SELECT orders.id, orders.qty FROM orders ORDER BY orders.qty DESC");
    LogicalPlanner planner;
    auto logical = planner.build_logical_plan(stmt);
    auto physical = build_physical_plan(logical.get(), catalog);

    auto rows = execute_plan(std::move(physical), nullptr);
    REQUIRE(rows.size() == 3);
    REQUIRE(rows[0][0] == "3");
    REQUIRE(rows[0][1] == "30");
    REQUIRE(rows[2][0] == "1");
}

TEST_CASE("Order by with limit returns top row", "[exec]") {
    Catalog catalog = build_orders_catalog();
    SelectStmt stmt = parse_sql("SELECT orders.id, orders.qty FROM orders ORDER BY orders.qty DESC LIMIT 1");
    LogicalPlanner planner;
    auto logical = planner.build_logical_plan(stmt);
    auto physical = build_physical_plan(logical.get(), catalog);

    auto rows = execute_plan(std::move(physical), nullptr);
    REQUIRE(rows.size() == 1);
    REQUIRE(rows[0][0] == "3");
    REQUIRE(rows[0][1] == "30");
}

TEST_CASE("Top region by quantity", "[exec]") {
    std::shared_ptr<Dictionary> detail_dict;
    Catalog catalog = build_full_catalog(detail_dict);
    SelectStmt stmt = parse_sql(
        "SELECT detail.region, SUM(orders.qty) AS total "
        "FROM orders INNER JOIN detail ON orders.id = detail.id "
        "GROUP BY detail.region ORDER BY total DESC LIMIT 1");
    LogicalPlanner planner;
    auto logical = planner.build_logical_plan(stmt);
    auto physical = build_physical_plan(logical.get(), catalog);

    Dictionary* dict = physical->dictionary();
    auto rows = execute_plan(std::move(physical), dict);
    REQUIRE(rows.size() == 1);
    REQUIRE(rows[0][0] == "south");
    REQUIRE(rows[0][1] == "20");
}
