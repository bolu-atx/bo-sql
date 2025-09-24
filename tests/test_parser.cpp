#include <catch2/catch_all.hpp>
#include "types.h"
#include "parser/parser.h"

TEST_CASE("Parser tokenizer test", "[parser]") {
    Parser p("SELECT id, name FROM users WHERE age > 18");
    auto& tokens = p.get_tokens();
    REQUIRE(tokens.size() == 11); // including END
    REQUIRE(tokens[0].type == TokenType::SELECT);
    REQUIRE(tokens[1].type == TokenType::IDENTIFIER);
    REQUIRE(tokens[1].value == "id");
    REQUIRE(tokens[2].type == TokenType::COMMA);
    REQUIRE(tokens[3].type == TokenType::IDENTIFIER);
    REQUIRE(tokens[3].value == "name");
    REQUIRE(tokens[4].type == TokenType::FROM);
    REQUIRE(tokens[5].type == TokenType::IDENTIFIER);
    REQUIRE(tokens[5].value == "users");
    REQUIRE(tokens[6].type == TokenType::WHERE);
    REQUIRE(tokens[7].type == TokenType::IDENTIFIER);
    REQUIRE(tokens[7].value == "age");
    REQUIRE(tokens[8].type == TokenType::GT);
    REQUIRE(tokens[9].type == TokenType::NUMBER);
    REQUIRE(tokens[9].value == "18");
    REQUIRE(tokens[10].type == TokenType::END);
}

TEST_CASE("Parser expression test", "[parser]") {
    // Test basic expression parsing
    SelectStmt stmt = parse_sql("SELECT 1 + 2 * 3 FROM t");
    REQUIRE(stmt.select_list.size() == 1);
    REQUIRE(stmt.select_list[0].expr->type == ExprType::BINARY_OP);
    REQUIRE(stmt.select_list[0].expr->op == BinaryOp::ADD);
    REQUIRE(stmt.select_list[0].expr->left->type == ExprType::LITERAL_INT);
    REQUIRE(stmt.select_list[0].expr->left->i64_val == 1);
    REQUIRE(stmt.select_list[0].expr->right->type == ExprType::BINARY_OP);
    REQUIRE(stmt.select_list[0].expr->right->op == BinaryOp::MUL);
}

TEST_CASE("Parser aggregate function test", "[parser]") {
    SelectStmt stmt = parse_sql("SELECT SUM(price), COUNT(*) FROM products");
    REQUIRE(stmt.select_list.size() == 2);
    REQUIRE(stmt.select_list[0].expr->type == ExprType::FUNC_CALL);
    REQUIRE(stmt.select_list[0].expr->func_name == "SUM");
    REQUIRE(stmt.select_list[0].expr->args.size() == 1);
    REQUIRE(stmt.select_list[0].expr->args[0]->type == ExprType::COLUMN_REF);
    REQUIRE(stmt.select_list[0].expr->args[0]->str_val == "price");
    
    REQUIRE(stmt.select_list[1].expr->type == ExprType::FUNC_CALL);
    REQUIRE(stmt.select_list[1].expr->func_name == "COUNT");
    REQUIRE(stmt.select_list[1].expr->args.size() == 1);
    REQUIRE(stmt.select_list[1].expr->args[0]->type == ExprType::COLUMN_REF);
    REQUIRE(stmt.select_list[1].expr->args[0]->str_val == "*");
}

TEST_CASE("Parser JOIN test", "[parser]") {
    // First test tokenization
    Parser p("SELECT name FROM products INNER JOIN orders ON id = pid");
    auto& tokens = p.get_tokens();
    // Find the ON part
    size_t on_idx = 0;
    for (size_t i = 0; i < tokens.size(); ++i) {
        if (tokens[i].type == TokenType::ON) {
            on_idx = i;
            break;
        }
    }
    REQUIRE(on_idx > 0);
    REQUIRE(tokens[on_idx + 1].value == "id");
    REQUIRE(tokens[on_idx + 2].value == "=");
    REQUIRE(tokens[on_idx + 3].value == "pid");

    // Now test parsing
    SelectStmt stmt = parse_sql("SELECT name FROM products INNER JOIN orders ON id = pid");

    // Check FROM table
    REQUIRE(stmt.from_table == "products");

    // Check JOIN
    REQUIRE(stmt.joins.size() == 1);
    REQUIRE(stmt.joins[0].table_name == "orders");
    REQUIRE(stmt.joins[0].on_condition->type == ExprType::BINARY_OP);
    REQUIRE(stmt.joins[0].on_condition->op == BinaryOp::EQ);
}

TEST_CASE("Parser error handling test", "[parser]") {
    // Test missing FROM
    REQUIRE_THROWS_AS(parse_sql("SELECT name"), std::runtime_error);
    
    // Test invalid token
    REQUIRE_THROWS_AS(parse_sql("SELECT @name FROM table"), std::runtime_error);
    
    // Test incomplete expression
    REQUIRE_THROWS_AS(parse_sql("SELECT name FROM table WHERE"), std::runtime_error);
}