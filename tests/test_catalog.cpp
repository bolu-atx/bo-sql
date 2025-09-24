#include <catch2/catch_all.hpp>
#include "storage/csv_loader.h"
#include "catalog/catalog.h"
#include <fstream>
#include "types.h"

TEST_CASE("Catalog roundtrip test", "[catalog]") {
    // Create a temporary CSV file
    std::ofstream csv_file("test_catalog.csv");
    csv_file << "id,value\n";
    csv_file << "10,1.1\n";
    csv_file << "20,2.2\n";
    csv_file.close();

    // Load and register
    std::pair<Table, TableMeta> result2 = load_csv("test_catalog.csv");
    Table& table = result2.first;
    TableMeta& meta = result2.second;
    table.name = "mytable";
    meta.name = "mytable";

    Catalog catalog;
    catalog.register_table(std::move(meta));

    // Retrieve from catalog
    const TableMeta* retrieved = catalog.get_table("mytable");
    REQUIRE(retrieved != nullptr);
    REQUIRE(retrieved->name == "mytable");
    REQUIRE(retrieved->row_count == 2);
    REQUIRE(retrieved->columns.size() == 2);

    REQUIRE(retrieved->columns[0].name == "id");
    REQUIRE(retrieved->columns[0].type == TypeId::INT64);
    REQUIRE(retrieved->columns[1].name == "value");
    REQUIRE(retrieved->columns[1].type == TypeId::DOUBLE);

    // Check table list
    auto tables = catalog.list_tables();
    REQUIRE(tables.size() == 1);
    REQUIRE(tables[0] == "mytable");

    // Clean up
    std::remove("test_catalog.csv");
}