#define CATCH_CONFIG_MAIN
#include <catch2/catch_all.hpp>
#include "../storage/csv_loader.h"
#include "../catalog/catalog.h"
#include <fstream>
#include "types.h"

TEST_CASE("Basic test", "[basic]") {
    REQUIRE(1 == 1);
}

TEST_CASE("ColumnVector smoke test", "[columnar]") {
    // Instantiate ColumnVector<int64_t>
    ColumnVector<i64> col(10);

    // Fill with values
    for (i64 i = 0; i < 5; ++i) {
        col.append(i * 10);
    }

    // Check size
    REQUIRE(col.size() == 5);

    // Check type
    REQUIRE(col.type() == TypeId::INT64);

    // Retrieve and check values
    REQUIRE(col.data[0] == 0);
    REQUIRE(col.data[1] == 10);
    REQUIRE(col.data[2] == 20);
    REQUIRE(col.data[3] == 30);
    REQUIRE(col.data[4] == 40);
}

TEST_CASE("Datum union test", "[types]") {
    // Test int64_t datum
    Datum d1 = Datum::from_i64(42);
    REQUIRE(d1.type == TypeId::INT64);
    REQUIRE(d1.as_i64() == 42);

    // Test double datum
    Datum d2 = Datum::from_f64(3.14);
    REQUIRE(d2.type == TypeId::DOUBLE);
    REQUIRE(d2.as_f64() == 3.14);

    // Test string ID datum
    Datum d3 = Datum::from_str(123);
    REQUIRE(d3.type == TypeId::STRING);
    REQUIRE(d3.as_str() == 123);

    // Test date32 datum
    Datum d4 = Datum::from_date32(20231225);
    REQUIRE(d4.type == TypeId::DATE32);
    REQUIRE(d4.as_date32() == 20231225);

    // Test type mismatch error
    REQUIRE_THROWS_AS(d1.as_f64(), std::runtime_error);
}

TEST_CASE("ColumnType test", "[types]") {
    // Test basic construction
    ColumnType ct1(TypeId::INT64, "id");
    REQUIRE(ct1.type_id == TypeId::INT64);
    REQUIRE(ct1.name == "id");

    // Test construction without name
    ColumnType ct2(TypeId::DOUBLE);
    REQUIRE(ct2.type_id == TypeId::DOUBLE);
    REQUIRE(ct2.name.empty());

    // Test equality
    ColumnType ct3(TypeId::INT64, "other_id");
    REQUIRE(ct1 == ct3);  // Same type, different name
    REQUIRE(ct1 != ct2);  // Different type
}

TEST_CASE("RecordBatch test", "[columnar]") {
    // Create schema
    std::vector<ColumnType> schema = {
        ColumnType(TypeId::INT64, "id"),
        ColumnType(TypeId::DOUBLE, "value")
    };

    // Create RecordBatch
    RecordBatch batch(schema);
    REQUIRE(batch.num_columns() == 0);
    REQUIRE(batch.num_rows() == 0);

    // Add columns
    std::unique_ptr<ColumnVector<i64> > col1(new ColumnVector<i64>());
    col1->append(1);
    col1->append(2);
    col1->append(3);

    std::unique_ptr<ColumnVector<f64> > col2(new ColumnVector<f64>());
    col2->append(1.1);
    col2->append(2.2);
    col2->append(3.3);

    batch.add_column(std::move(col1));
    batch.add_column(std::move(col2));

    // Check dimensions
    REQUIRE(batch.num_columns() == 2);
    REQUIRE(batch.num_rows() == 3);

    // Check column access
    ColumnVector<i64>* c1 = dynamic_cast<ColumnVector<i64>*>(batch.get_column(0));
    ColumnVector<f64>* c2 = dynamic_cast<ColumnVector<f64>*>(batch.get_column(1));

    REQUIRE(c1 != nullptr);
    REQUIRE(c2 != nullptr);
    REQUIRE(c1->data[0] == 1);
    REQUIRE(c2->data[1] == 2.2);

    // Check schema access
    REQUIRE(batch.get_column_type(0).name == "id");
    REQUIRE(batch.get_column_type(1).type_id == TypeId::DOUBLE);
}

TEST_CASE("CSV load test", "[csv]") {
    // Create a temporary CSV file
    std::ofstream csv_file("test_load.csv");
    csv_file << "id,name,value\n";
    csv_file << "1,Alice,100.5\n";
    csv_file << "2,Bob,200.25\n";
    csv_file << "3,Charlie,300.75\n";
    csv_file.close();

    // Load CSV
    std::pair<Table, TableMeta> result = load_csv("test_load.csv");
    Table& table = result.first;
    TableMeta& meta = result.second;

    // Check table metadata
    REQUIRE(meta.name.empty()); // Not set yet
    REQUIRE(meta.row_count == 3);
    REQUIRE(meta.columns.size() == 3);

    // Check columns
    REQUIRE(meta.columns[0].name == "id");
    REQUIRE(meta.columns[0].type == TypeId::INT64);
    REQUIRE(meta.columns[0].stats.min_i64 == 1);
    REQUIRE(meta.columns[0].stats.max_i64 == 3);

    REQUIRE(meta.columns[1].name == "name");
    REQUIRE(meta.columns[1].type == TypeId::STRING);
    REQUIRE(meta.columns[1].stats.ndv == 3);

    REQUIRE(meta.columns[2].name == "value");
    REQUIRE(meta.columns[2].type == TypeId::DOUBLE);
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