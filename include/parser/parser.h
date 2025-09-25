#pragma once

#include <string>
#include <vector>
#include "parser/ast.h"

namespace bosql {

enum class TokenType {
    SELECT, FROM, WHERE, INNER, JOIN, ON, GROUP, BY, HAVING, ORDER, ASC, DESC, LIMIT,
    IDENTIFIER, NUMBER, STRING_LITERAL, COMMA, LPAREN, RPAREN, EQ, NE, LT, LE, GT, GE, PLUS, MINUS, MUL, DIV,
    SUM, COUNT, AVG, AS, AND, OR,
    END
};

struct Token {
    TokenType type;
    std::string value;
};

class Parser {
public:
    Parser(const std::string& sql);
    SelectStmt parse();

    // For testing
    const std::vector<Token>& get_tokens() const { return tokens_; }

private:
    std::string sql_;
    size_t pos_;
    std::vector<Token> tokens_;

    void tokenize();
    TokenType get_keyword(const std::string& s);
    Token current();
    void advance();
    SelectStmt parse_select();
    std::vector<SelectItem> parse_select_list();
    std::unique_ptr<Expr> parse_expr();
    std::unique_ptr<Expr> parse_predicate();
    std::unique_ptr<Expr> parse_or_expr();
    std::unique_ptr<Expr> parse_and_expr();
    std::unique_ptr<Expr> parse_cmp_expr();
    std::unique_ptr<Expr> parse_add_expr();
    std::unique_ptr<Expr> parse_mul_expr();
    std::unique_ptr<Expr> parse_factor();
    std::unique_ptr<Expr> parse_primary();
    std::vector<std::unique_ptr<Expr> > parse_expr_list();
    std::vector<OrderByItem> parse_order_list();
    bool is_cmp_op(TokenType t);
    BinaryOp token_to_cmp_op(TokenType t);
    Token expect(TokenType type);
};

SelectStmt parse_sql(const std::string& sql);

} // namespace bosql