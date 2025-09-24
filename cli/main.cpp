#include <iostream>
#include <string>
#include <sstream>
#include <utility>
#include <fmt/core.h>
#include <fmt/color.h>
#include "../catalog/catalog.h"
#include "../storage/csv_loader.h"
#include "../include/types.h"

template<typename... Args>
void print_info(std::string_view fmt, Args&&... args) { fmt::print("{}\n", fmt::vformat(fmt, fmt::make_format_args(std::forward<Args>(args)...))); }

template<typename... Args>
void print_success(std::string_view fmt, Args&&... args) { fmt::print(fg(fmt::color::green), "{}\n", fmt::vformat(fmt, fmt::make_format_args(std::forward<Args>(args)...))); }

template<typename... Args>
void print_warning(std::string_view fmt, Args&&... args) { fmt::print(fg(fmt::color::yellow), "{}\n", fmt::vformat(fmt, fmt::make_format_args(std::forward<Args>(args)...))); }

template<typename... Args>
void print_error(std::string_view fmt, Args&&... args) { fmt::print(fg(fmt::color::red), "{}\n", fmt::vformat(fmt, fmt::make_format_args(std::forward<Args>(args)...))); }

std::string type_name(TypeId type) {
    switch (type) {
        case TypeId::INT64: return "INT64";
        case TypeId::DOUBLE: return "DOUBLE";
        case TypeId::STRING: return "STRING";
        case TypeId::DATE32: return "DATE32";
        default: return "UNKNOWN";
    }
}

int main() {
    Catalog catalog;
    std::string line;

    fmt::print("bo-sql CLI\n> ");

    while (std::getline(std::cin, line)) {
        std::istringstream iss(line);
        std::string command;
        iss >> command;

        if (command == "LOAD") {
            std::string table_keyword, table_name, from_keyword, filename;
            iss >> table_keyword >> table_name >> from_keyword >> filename;
            if (table_keyword != "TABLE" || from_keyword != "FROM") {
                print_warning("Syntax: LOAD TABLE <name> FROM 'file.csv'");
            } else {
                // Remove quotes from filename
                if (!filename.empty() && filename.front() == '\'' && filename.back() == '\'') {
                    filename = filename.substr(1, filename.size() - 2);
                }
                try {
                    std::pair<Table, TableMeta> result = load_csv(filename);
                result.first.name = table_name;
                result.second.name = table_name;
                catalog.register_table(std::move(result.second));

                    print_success("Loaded table '{}' with {} rows", table_name, result.second.row_count);
                } catch (const std::exception& e) {
                    print_error("Error loading CSV: {}", e.what());
                }
            }
        } else if (command == "SHOW") {
            std::string tables_keyword;
            iss >> tables_keyword;
            if (tables_keyword == "TABLES") {
                auto tables = catalog.list_tables();
                if (tables.empty()) {
                    print_info("No tables loaded");
                } else {
                for (const auto& table : tables) {
                    fmt::print("{}\n", table);
                }
                }
            } else {
                print_warning("Unknown command");
            }
        } else if (command == "DESCRIBE") {
            std::string table_name;
            iss >> table_name;
            const TableMeta* meta = catalog.get_table(table_name);
            if (!meta) {
                print_error("Table '{}' not found", table_name);
            } else {
                 fmt::print("Table: {} ({} rows)\n", meta->name, meta->row_count);
                 fmt::print("Columns:\n");
                 for (const auto& col : meta->columns) {
                     fmt::print("  {} {} (ndv: {}", col.name, type_name(col.type), col.stats.ndv);
                     if (col.type == TypeId::INT64) {
                         fmt::print(", min: {}, max: {}", col.stats.min_i64, col.stats.max_i64);
                     } else if (col.type == TypeId::DOUBLE) {
                         fmt::print(", min: {}, max: {}", col.stats.min_f64, col.stats.max_f64);
                     } else if (col.type == TypeId::DATE32) {
                         fmt::print(", min: {}, max: {}", col.stats.min_date, col.stats.max_date);
                     }
                     fmt::print(")\n");
                 }
            }
        } else if (command == "EXIT" || command == "QUIT") {
            break;
        } else {
            print_warning("Unknown command. Available: LOAD TABLE, SHOW TABLES, DESCRIBE <table>, EXIT");
        }

        fmt::print("> ");
    }

    return 0;
}