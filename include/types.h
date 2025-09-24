#pragma once

#include <cstdint>
#include <memory>
#include <stdexcept>
#include <vector>

using i64 = int64_t;
using f64 = double;
using StrId = uint32_t;  // Dictionary-encoded string ID
using Date32 = int32_t;  // YYYYMMDD format

// Enumeration of supported data types
enum class TypeId { INT64, DOUBLE, STRING, DATE32 };

// Datum union for type-safe value storage
union DatumValue {
    int64_t i64_val;
    double f64_val;
    StrId str_id;
    Date32 date32_val;
};

// Type-safe wrapper for a single value with its type
struct Datum {
    TypeId type;
    DatumValue value;

    // Type-safe accessors
    int64_t as_i64() const {
        if (type != TypeId::INT64) throw std::runtime_error("Type mismatch");
        return value.i64_val;
    }

    double as_f64() const {
        if (type != TypeId::DOUBLE) throw std::runtime_error("Type mismatch");
        return value.f64_val;
    }

    StrId as_str() const {
        if (type != TypeId::STRING) throw std::runtime_error("Type mismatch");
        return value.str_id;
    }

    Date32 as_date32() const {
        if (type != TypeId::DATE32) throw std::runtime_error("Type mismatch");
        return value.date32_val;
    }

    // Static constructors
    static Datum from_i64(int64_t v) {
        return {TypeId::INT64, {.i64_val = v}};
    }

    static Datum from_f64(double v) {
        return {TypeId::DOUBLE, {.f64_val = v}};
    }

    static Datum from_str(StrId v) {
        return {TypeId::STRING, {.str_id = v}};
    }

    static Datum from_date32(Date32 v) {
        return {TypeId::DATE32, {.date32_val = v}};
    }
};

// Column type metadata
struct ColumnType {
    TypeId type_id;
    std::string name;  // Optional column name

    ColumnType(TypeId id, std::string col_name = "")
        : type_id(id), name(std::move(col_name)) {}

    bool operator==(const ColumnType& other) const {
        return type_id == other.type_id;
    }

    bool operator!=(const ColumnType& other) const {
        return !(*this == other);
    }
};

// Type mapping helper
template<typename T>
TypeId type_id_for();

template<>
inline TypeId type_id_for<int64_t>() { return TypeId::INT64; }

template<>
inline TypeId type_id_for<double>() { return TypeId::DOUBLE; }

template<>
inline TypeId type_id_for<int32_t>() { return TypeId::DATE32; }

template<>
inline TypeId type_id_for<uint32_t>() { return TypeId::STRING; }

// Base class
struct Column {
    virtual ~Column() {}
    virtual TypeId type() const = 0;
    virtual size_t size() const = 0;
};

// Typed column
template<typename T>
struct ColumnVector : public Column {
    std::vector<T> data;

    explicit ColumnVector(size_t reserve = 0) { data.reserve(reserve); }
    explicit ColumnVector(std::vector<T> d) : data(std::move(d)) {}

    TypeId type() const override { return type_id_for<T>(); }
    size_t size() const override { return data.size(); }

    void append(const T& v) { data.push_back(v); }
};

// RecordBatch abstraction
struct RecordBatch {
    std::vector<ColumnType> schema;
    std::vector<std::unique_ptr<Column> > columns;

    RecordBatch(std::vector<ColumnType> s) : schema(std::move(s)) {
        columns.reserve(schema.size());
    }

    size_t num_rows() const {
        return columns.empty() ? 0 : columns[0]->size();
    }

    size_t num_columns() const {
        return columns.size();
    }

    // Add a column to the batch
    template<typename T>
    void add_column(std::unique_ptr<ColumnVector<T> > col) {
        columns.push_back(std::move(col));
    }

    // Get a column by index
    Column* get_column(size_t index) const {
        return columns.at(index).get();
    }

    // Get column type by index
    const ColumnType& get_column_type(size_t index) const {
        return schema.at(index);
    }
};