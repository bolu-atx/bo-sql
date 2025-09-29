#pragma once

#include <cstddef>
#include <iosfwd>
#include <string>
#include <vector>
#include "types.h"

namespace bosql {

struct Formatter {
    explicit Formatter(std::ostream& stream) : out(stream) {}
    virtual ~Formatter() = default;
    virtual void begin(const std::vector<std::string>& names, const std::vector<TypeId>& types) = 0;
    virtual void write_row(std::vector<std::string> row) = 0;
    virtual void end(std::size_t row_count) = 0;

protected:
    std::ostream& out;
};

struct MarkdownFormatter final : Formatter {
    explicit MarkdownFormatter(std::ostream& stream);
    void begin(const std::vector<std::string>& names, const std::vector<TypeId>& types) override;
    void write_row(std::vector<std::string> row) override;
    void end(std::size_t row_count) override;

private:
    std::vector<std::string> headers;
    std::vector<std::vector<std::string>> data;
    std::vector<std::size_t> widths;
};

struct CsvFormatter final : Formatter {
    explicit CsvFormatter(std::ostream& stream, char delimiter = ',');
    void begin(const std::vector<std::string>& names, const std::vector<TypeId>& types) override;
    void write_row(std::vector<std::string> row) override;
    void end(std::size_t row_count) override;

private:
    char sep;
    bool header_written = false;
    std::vector<std::string> headers;
    std::string escape_cell(const std::string& cell) const;
};

} // namespace bosql
