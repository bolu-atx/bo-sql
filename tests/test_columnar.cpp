#include <catch2/catch_all.hpp>
#include "types.h"

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