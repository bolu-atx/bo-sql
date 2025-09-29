#include <algorithm>
#include <cctype>
#include <iostream>
#include <string>
#include <sstream>
#include <utility>
#include <string_view>
#include <fmt/core.h>
#include <fmt/color.h>
#include "catalog/catalog.h"
#include "storage/csv_loader.h"
#include "parser/parser.h"
#include "logical/planner.h"
#include "exec/physical_planner.h"
#include "exec/formatter.hpp"
#include "types.h"

template<typename... Args>
void print_info(std::string_view fmt, Args const&... args) { fmt::print("{}\n", fmt::vformat(fmt, fmt::make_format_args(args...))); }

template<typename... Args>
void print_success(std::string_view fmt, Args const&... args) { fmt::print(fg(fmt::color::green), "{}\n", fmt::vformat(fmt, fmt::make_format_args(args...))); }

template<typename... Args>
void print_warning(std::string_view fmt, Args const&... args) { fmt::print(fg(fmt::color::yellow), "{}\n", fmt::vformat(fmt, fmt::make_format_args(args...))); }

template<typename... Args>
void print_error(std::string_view fmt, Args const&... args) { fmt::print(fg(fmt::color::red), "{}\n", fmt::vformat(fmt, fmt::make_format_args(args...))); }

std::string type_name(bosql::TypeId type) {
    switch (type) {
        case bosql::TypeId::INT64: return "INT64";
        case bosql::TypeId::DOUBLE: return "DOUBLE";
        case bosql::TypeId::STRING: return "STRING";
        case bosql::TypeId::DATE32: return "DATE32";
        default: return "UNKNOWN";
    }
}

void execute_select_sql(const std::string& sql, bosql::Catalog& catalog, std::string_view output_format) {
    try {
        bosql::SelectStmt stmt = bosql::parse_sql(sql);
        bosql::LogicalPlanner planner;
        auto logical = planner.build_logical_plan(stmt);
        auto physical = bosql::build_physical_plan(logical.get(), catalog);
        auto [col_names, col_types, dict] = bosql::get_output_schema(logical.get(), catalog);
        if (output_format == "csv") {
            bosql::CsvFormatter formatter(std::cout);
            bosql::run_query(std::move(physical), col_names, col_types, formatter, dict);
        } else {
            bosql::MarkdownFormatter formatter(std::cout);
            bosql::run_query(std::move(physical), col_names, col_types, formatter, dict);
        }
    } catch (const std::exception& e) {
        print_error("Error: {}", e.what());
    }
}

int main(int argc, char* argv[]) {
    std::vector<std::string> args(argv + 1, argv + argc);
    std::string csv_file;
    bool sql_stdin = false;
    std::string output_format = "markdown";
    for (size_t i = 0; i < args.size(); ++i) {
        if (args[i] == "--sql") {
            sql_stdin = true;
        } else if (args[i] == "--output-format") {
            if (i + 1 < args.size()) {
                output_format = args[i + 1];
                std::transform(output_format.begin(), output_format.end(), output_format.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
                ++i;
            } else {
                print_error("--output-format requires an argument");
                return 1;
            }
        } else if (args[i].starts_with("--")) {
            print_error("Unknown option: {}", args[i]);
            return 1;
        } else {
            if (csv_file.empty()) {
                csv_file = args[i];
            } else {
                print_error("Too many positional arguments");
                return 1;
            }
        }
    }

    if (output_format != "markdown" && output_format != "csv") {
        print_error("Unsupported output format '{}'. Use 'markdown' or 'csv'.", output_format);
        return 1;
    }

    bosql::Catalog catalog;

    if (sql_stdin) {
        // Read SQL from stdin first
        std::string sql;
        std::getline(std::cin, sql);
        // Load CSV
        std::string table_name = "table";
        if (!csv_file.empty()) {
            try {
                auto [table, meta] = bosql::load_csv(csv_file);
                table.name = table_name;
                meta.name = table_name;
                catalog.register_table(std::move(table), std::move(meta));
            } catch (const std::exception& e) {
                print_error("Error loading CSV: {}", e.what());
                return 1;
            }
        } else {
            std::stringstream csv_stream;
            csv_stream << std::cin.rdbuf();
            try {
                auto [table, meta] = bosql::load_csv(csv_stream);
                table.name = table_name;
                meta.name = table_name;
                catalog.register_table(std::move(table), std::move(meta));
            } catch (const std::exception& e) {
                print_error("Error loading CSV from stdin: {}", e.what());
                return 1;
            }
        }
        execute_select_sql(sql, catalog, output_format);
        return 0;
    } else {
        // Load CSV if provided
        if (!csv_file.empty()) {
            std::string table_name = "table";
            try {
                auto [table, meta] = bosql::load_csv(csv_file);
                table.name = table_name;
                meta.name = table_name;
                catalog.register_table(std::move(table), std::move(meta));
                print_success("Loaded table from {}", csv_file);
            } catch (const std::exception& e) {
                print_error("Error loading CSV: {}", e.what());
                return 1;
            }
        }
        // REPL
        std::string line;
        fmt::print("bq CLI\n> ");
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
                    std::pair<bosql::Table, bosql::TableMeta> result = bosql::load_csv(filename);
                result.first.name = table_name;
                result.second.name = table_name;
                     catalog.register_table(std::move(result.first), std::move(result.second));

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
               bosql::OptionalRef<const bosql::TableMeta> meta = catalog.get_table_meta(table_name);
              if (!meta.has_value()) {
                  print_error("Table '{}' not found", table_name);
              } else {
                  fmt::print("Table: {} ({} rows)\n", meta->name, meta->row_count);
                  fmt::print("Columns:\n");
                  for (const auto& col : meta->columns) {
                      fmt::print("  {} {} (ndv: {}", col.name, type_name(col.type), col.stats.ndv);
                      if (col.type == bosql::TypeId::INT64) {
                          fmt::print(", min: {}, max: {}", col.stats.min_i64, col.stats.max_i64);
                      } else if (col.type == bosql::TypeId::DOUBLE) {
                          fmt::print(", min: {}, max: {}", col.stats.min_f64, col.stats.max_f64);
                      } else if (col.type == bosql::TypeId::DATE32) {
                          fmt::print(", min: {}, max: {}", col.stats.min_date, col.stats.max_date);
                      }
                      fmt::print(")\n");
                  }
             }
        } else if (command == "EXPLAIN") {
            std::string sql;
            std::getline(iss, sql);
            // Remove leading spaces
            size_t start = sql.find_first_not_of(" \t");
             if (start != std::string::npos) {
                 sql = sql.substr(start);
             }
             if (sql.empty()) {
                 print_warning("Syntax: EXPLAIN <sql>");
              } else {
                  try {
                      bosql::SelectStmt stmt = bosql::parse_sql(sql);
                      bosql::LogicalPlanner planner;
                      auto plan = planner.build_logical_plan(stmt);
                      fmt::print("{}\n", plan->to_string());
                  } catch (const std::exception& e) {
                      print_error("Error: {}", e.what());
                  }
              }
          } else if (command == "SELECT") {
              std::string sql;
              std::getline(iss, sql);
              sql = "SELECT " + sql;
              // Remove leading spaces
              size_t start = sql.find_first_not_of(" \t");
              if (start != std::string::npos) {
                  sql = sql.substr(start);
              }
               if (sql.empty()) {
                   print_warning("Syntax: SELECT <sql>");
                } else {
              execute_select_sql(sql, catalog, output_format);
                }
         } else if (command == "EXIT" || command == "QUIT") {
            break;
         } else if (command == "SET") {
            std::string setting;
            iss >> setting;
            if (setting == "FORMAT") {
                std::string value;
                iss >> value;
                if (value.empty()) {
                    print_warning("Syntax: SET FORMAT <markdown|csv>");
                } else {
                    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
                    if (value == "markdown" || value == "csv") {
                        output_format = value;
                        print_success("Output format set to {}", output_format);
                    } else {
                        print_warning("Unsupported output format '{}'. Use 'markdown' or 'csv'.", value);
                    }
                }
            } else {
                print_warning("Unknown setting");
            }
         } else {
             print_warning("Unknown command. Available: LOAD TABLE, SHOW TABLES, DESCRIBE <table>, EXPLAIN <sql>, SELECT <sql>, SET FORMAT <markdown|csv>, EXIT");
         }

        fmt::print("> ");
    }
    }

    return 0;
}
