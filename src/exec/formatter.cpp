#include "exec/formatter.hpp"
#include <algorithm>
#include <iomanip>
#include <ostream>
#include <fmt/core.h>

namespace bosql {

MarkdownFormatter::MarkdownFormatter(std::ostream& stream) : Formatter(stream) {}

void MarkdownFormatter::begin(const std::vector<std::string>& names, const std::vector<TypeId>& types) {
    (void)types;
    headers = names;
    data.clear();
    widths.assign(headers.size(), 0);
    for (std::size_t i = 0; i < headers.size(); ++i) {
        widths[i] = std::max<std::size_t>(headers[i].size(), widths[i]);
    }
}

void MarkdownFormatter::write_row(std::vector<std::string> row) {
    for (std::size_t i = 0; i < row.size(); ++i) {
        if (i >= widths.size()) {
            widths.resize(i + 1, 0);
        }
        widths[i] = std::max<std::size_t>(widths[i], row[i].size());
    }
    data.push_back(std::move(row));
}

void MarkdownFormatter::end(std::size_t row_count) {
    if (row_count == 0) {
        out << "(no results)\n";
        return;
    }
    if (headers.empty()) {
        headers.resize(widths.size());
        for (std::size_t i = 0; i < headers.size(); ++i) {
            headers[i] = fmt::format("col{}", i + 1);
            if (i >= widths.size()) {
                widths.push_back(headers[i].size());
            } else {
                widths[i] = std::max<std::size_t>(widths[i], headers[i].size());
            }
        }
    }
    auto flags = out.flags();
    auto print_row = [&](const std::vector<std::string>& cells) {
        out << "|";
        for (std::size_t i = 0; i < widths.size(); ++i) {
            out << " " << std::left << std::setw(static_cast<int>(widths[i])) << (i < cells.size() ? cells[i] : std::string()) << " |";
        }
        out << '\n';
    };

    print_row(headers);
    out << "|";
    for (std::size_t i = 0; i < widths.size(); ++i) {
        out << " " << std::string(widths[i], '-') << " |";
    }
    out << '\n';
    for (const auto& row : data) {
        print_row(row);
    }
    out.flags(flags);
}

CsvFormatter::CsvFormatter(std::ostream& stream, char delimiter) : Formatter(stream), sep(delimiter) {}

void CsvFormatter::begin(const std::vector<std::string>& names, const std::vector<TypeId>& types) {
    (void)types;
    headers = names;
    header_written = false;
    if (headers.empty()) {
        return;
    }
    bool first = true;
    for (const auto& name : headers) {
        if (!first) {
            out << sep;
        }
        out << escape_cell(name);
        first = false;
    }
    out << '\n';
    header_written = true;
}

void CsvFormatter::write_row(std::vector<std::string> row) {
    if (headers.empty() && !header_written) {
        headers.resize(row.size());
        header_written = true;
    }
    bool first = true;
    for (const auto& cell : row) {
        if (!first) {
            out << sep;
        }
        out << escape_cell(cell);
        first = false;
    }
    out << '\n';
}

void CsvFormatter::end(std::size_t row_count) {
    (void)row_count;
}

std::string CsvFormatter::escape_cell(const std::string& cell) const {
    bool needs_quotes = cell.find(sep) != std::string::npos ||
                        cell.find('"') != std::string::npos ||
                        cell.find('\n') != std::string::npos ||
                        cell.find('\r') != std::string::npos;
    if (!needs_quotes) {
        return cell;
    }
    std::string escaped;
    escaped.reserve(cell.size() + 2);
    escaped.push_back('"');
    for (char ch : cell) {
        if (ch == '"') {
            escaped.push_back('"');
        }
        escaped.push_back(ch);
    }
    escaped.push_back('"');
    return escaped;
}

} // namespace bosql
