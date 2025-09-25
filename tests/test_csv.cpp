#include <catch2/catch_all.hpp>
#include "storage/csv_loader.h"
#include "catalog/catalog.h"
#include <fstream>
#include "types.h"

TEST_CASE("CSV load test", "[csv]") {
    // Create a temporary CSV file
    std::ofstream csv_file("test_load.csv");
    csv_file << "id,name,value\n";
    csv_file << "1,Alice,100.5\n";
    csv_file << "2,Bob,200.25\n";
    csv_file << "3,Charlie,300.75\n";
    csv_file.close();

    // Load CSV
    std::pair<bosql::Table, bosql::TableMeta> result = bosql::load_csv("test_load.csv");
    bosql::Table& table = result.first;
    bosql::TableMeta& meta = result.second;

    // Check table metadata
    REQUIRE(meta.name.empty()); // Not set yet
    REQUIRE(meta.row_count == 3);
    REQUIRE(meta.columns.size() == 3);

    // Check columns
    REQUIRE(meta.columns[0].name == "id");
    REQUIRE(meta.columns[0].type == bosql::TypeId::INT64);
    REQUIRE(meta.columns[0].stats.min_i64 == 1);
    REQUIRE(meta.columns[0].stats.max_i64 == 3);

    REQUIRE(meta.columns[1].name == "name");
    REQUIRE(meta.columns[1].type == bosql::TypeId::STRING);
    REQUIRE(meta.columns[1].stats.ndv == 3);

    REQUIRE(meta.columns[2].name == "value");
    REQUIRE(meta.columns[2].type == bosql::TypeId::DOUBLE);
    REQUIRE(meta.columns[2].stats.min_f64 == 100.5);
    REQUIRE(meta.columns[2].stats.max_f64 == 300.75);

    // Check table data
    REQUIRE(table.columns.size() == 3);
    REQUIRE(table.columns[0].name == "id");
    REQUIRE(table.columns[1].name == "name");
    REQUIRE(table.columns[2].name == "value");

    // Check dictionary
    REQUIRE(table.dict->get(0) == "Alice");
    REQUIRE(table.dict->get(1) == "Bob");
    REQUIRE(table.dict->get(2) == "Charlie");

    // Clean up
    std::remove("test_load.csv");
}