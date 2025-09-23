#include <catch2/catch_test_macros.hpp>
#include "types.h"

TEST_CASE("Basic test", "[basic]") {
    REQUIRE(1 == 1);
}

TEST_CASE("ColumnVector smoke test", "[columnar]") {
    // Instantiate ColumnVector<int64_t>
    ColumnVector<int64_t> col(10);

    // Fill with values
    for (int64_t i = 0; i < 5; ++i) {
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
    auto d1 = Datum::from_i64(42);
    REQUIRE(d1.type == TypeId::INT64);
    REQUIRE(d1.as_i64() == 42);

    // Test double datum
    auto d2 = Datum::from_f64(3.14);
    REQUIRE(d2.type == TypeId::DOUBLE);
    REQUIRE(d2.as_f64() == 3.14);

    // Test string ID datum
    auto d3 = Datum::from_str(123);
    REQUIRE(d3.type == TypeId::STRING);
    REQUIRE(d3.as_str() == 123);

    // Test date32 datum
    auto d4 = Datum::from_date32(20231225);
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
    auto col1 = std::make_unique<ColumnVector<int64_t>>();
    col1->append(1);
    col1->append(2);
    col1->append(3);

    auto col2 = std::make_unique<ColumnVector<double>>();
    col2->append(1.1);
    col2->append(2.2);
    col2->append(3.3);

    batch.add_column(std::move(col1));
    batch.add_column(std::move(col2));

    // Check dimensions
    REQUIRE(batch.num_columns() == 2);
    REQUIRE(batch.num_rows() == 3);

    // Check column access
    auto* c1 = dynamic_cast<ColumnVector<int64_t>*>(batch.get_column(0));
    auto* c2 = dynamic_cast<ColumnVector<double>*>(batch.get_column(1));

    REQUIRE(c1 != nullptr);
    REQUIRE(c2 != nullptr);
    REQUIRE(c1->data[0] == 1);
    REQUIRE(c2->data[1] == 2.2);

    // Check schema access
    REQUIRE(batch.get_column_type(0).name == "id");
    REQUIRE(batch.get_column_type(1).type_id == TypeId::DOUBLE);
}