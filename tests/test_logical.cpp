#include <catch2/catch_all.hpp>
#include "parser/parser.h"
#include "logical/planner.h"

TEST_CASE("Logical plan - scan only", "[logical]") {
    SelectStmt stmt = parse_sql("SELECT a, b FROM t");
    LogicalPlanner planner;
    auto plan = planner.build_logical_plan(stmt);
    std::string result = plan->to_string();
    REQUIRE(result == "LogicalProject(a, b)\n  LogicalScan(table=t, cols=a, b)");
}

TEST_CASE("Logical plan - filter", "[logical]") {
    SelectStmt stmt = parse_sql("SELECT a FROM t WHERE b > 10");
    LogicalPlanner planner;
    auto plan = planner.build_logical_plan(stmt);
    std::string result = plan->to_string();
    REQUIRE(result == "LogicalProject(a)\n  LogicalFilter((b > 10))\n    LogicalScan(table=t, cols=a, b)");
}

TEST_CASE("Logical plan - join", "[logical]") {
    SelectStmt stmt = parse_sql("SELECT a FROM t1 INNER JOIN t2 ON t1.id = t2.id");
    LogicalPlanner planner;
    auto plan = planner.build_logical_plan(stmt);
    std::string result = plan->to_string();
    REQUIRE(result == "LogicalProject(a)\n  LogicalHashJoin(left_keys=t1.id, right_keys=t2.id)\n    LogicalScan(table=t1, cols=a, t1.id, t2.id)\n    LogicalScan(table=t2, cols=a, t1.id, t2.id)");
}

TEST_CASE("Logical plan - aggregate", "[logical]") {
    SelectStmt stmt = parse_sql("SELECT SUM(a) FROM t GROUP BY b");
    LogicalPlanner planner;
    auto plan = planner.build_logical_plan(stmt);
    std::string result = plan->to_string();
    REQUIRE(result == "LogicalProject(SUM(a))\n  LogicalAggregate(keys=b, aggs=SUM(a))\n    LogicalScan(table=t, cols=a, b)");
}

TEST_CASE("Logical plan - order", "[logical]") {
    SelectStmt stmt = parse_sql("SELECT a FROM t ORDER BY b DESC");
    LogicalPlanner planner;
    auto plan = planner.build_logical_plan(stmt);
    std::string result = plan->to_string();
    REQUIRE(result == "LogicalOrder(by: b DESC)\n  LogicalProject(a)\n    LogicalScan(table=t, cols=a, b)");
}

TEST_CASE("Logical plan - limit", "[logical]") {
    SelectStmt stmt = parse_sql("SELECT a FROM t LIMIT 5");
    LogicalPlanner planner;
    auto plan = planner.build_logical_plan(stmt);
    std::string result = plan->to_string();
    REQUIRE(result == "LogicalLimit(5)\n  LogicalProject(a)\n    LogicalScan(table=t, cols=a)");
}

TEST_CASE("Logical plan - complex", "[logical]") {
    SelectStmt stmt = parse_sql("SELECT sku, SUM(qty) FROM lineitem WHERE qty > 10 GROUP BY sku ORDER BY SUM(qty) DESC LIMIT 5");
    LogicalPlanner planner;
    auto plan = planner.build_logical_plan(stmt);
    std::string result = plan->to_string();
    std::string expected = "LogicalLimit(5)\n  LogicalOrder(by: SUM(qty) DESC)\n    LogicalProject(sku, SUM(qty))\n      LogicalAggregate(keys=sku, aggs=SUM(qty))\n        LogicalFilter((qty > 10))\n          LogicalScan(table=lineitem, cols=qty, sku)";
    REQUIRE(result == expected);
}