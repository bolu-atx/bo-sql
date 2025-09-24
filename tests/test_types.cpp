#include <catch2/catch_all.hpp>
#include "types.h"

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