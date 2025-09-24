#pragma once

#include <string>
#include <vector>
#include <sstream>
#include <cctype>
#include "parser/ast.h"

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
    Parser(const std::string& sql) : sql_(sql), pos_(0) {
        tokenize();
    }

    SelectStmt parse() {
        return parse_select();
    }

    // For testing
    const std::vector<Token>& get_tokens() const { return tokens_; }

private:
    std::string sql_;
    size_t pos_;
    std::vector<Token> tokens_;

    void tokenize() {
        std::string token;
        for (size_t i = 0; i < sql_.size(); ++i) {
            char c = sql_[i];
            if (std::isspace(c)) continue;
            if (std::isalpha(c) || c == '_') {
                token += c;
                while (i + 1 < sql_.size() && (std::isalnum(sql_[i+1]) || sql_[i+1] == '_')) {
                    token += sql_[++i];
                }
                TokenType type = get_keyword(token);
                tokens_.push_back({type, token});
                token.clear();
            } else if (std::isdigit(c)) {
                token += c;
                while (i + 1 < sql_.size() && std::isdigit(sql_[i+1])) {
                    token += sql_[++i];
                }
                tokens_.push_back({TokenType::NUMBER, token});
                token.clear();
            } else if (c == '\'') {
                token += c;
                while (i + 1 < sql_.size() && sql_[++i] != '\'') {
                    token += sql_[i];
                }
                token += '\'';
                tokens_.push_back({TokenType::STRING_LITERAL, token});
                token.clear();
            } else {
                // operators
                switch (c) {
                    case ',': tokens_.push_back({TokenType::COMMA, ","}); break;
                    case '(': tokens_.push_back({TokenType::LPAREN, "("}); break;
                    case ')': tokens_.push_back({TokenType::RPAREN, ")"}); break;
                    case '=': tokens_.push_back({TokenType::EQ, "="}); break;
                    case '<': 
                        if (i + 1 < sql_.size() && sql_[i+1] == '=') {
                            tokens_.push_back({TokenType::LE, "<="}); ++i;
                        } else {
                            tokens_.push_back({TokenType::LT, "<"});
                        }
                        break;
                    case '>': 
                        if (i + 1 < sql_.size() && sql_[i+1] == '=') {
                            tokens_.push_back({TokenType::GE, ">="}); ++i;
                        } else {
                            tokens_.push_back({TokenType::GT, ">"});
                        }
                        break;
                    case '!': 
                        if (i + 1 < sql_.size() && sql_[i+1] == '=') {
                            tokens_.push_back({TokenType::NE, "!="}); ++i;
                        }
                        break;
                    case '+': tokens_.push_back({TokenType::PLUS, "+"}); break;
                    case '-': tokens_.push_back({TokenType::MINUS, "-"}); break;
                    case '*': tokens_.push_back({TokenType::MUL, "*"}); break;
                    case '/': tokens_.push_back({TokenType::DIV, "/"}); break;
                    case '.': tokens_.push_back({TokenType::IDENTIFIER, "."}); break; // Handle qualified names
                    default: throw std::runtime_error("Unknown token: " + std::string(1, c));
                }
            }
        }
        tokens_.push_back({TokenType::END, ""});
    }

    TokenType get_keyword(const std::string& s) {
        if (s == "SELECT") return TokenType::SELECT;
        if (s == "FROM") return TokenType::FROM;
        if (s == "WHERE") return TokenType::WHERE;
        if (s == "INNER") return TokenType::INNER;
        if (s == "JOIN") return TokenType::JOIN;
        if (s == "ON") return TokenType::ON;
        if (s == "GROUP") return TokenType::GROUP;
        if (s == "BY") return TokenType::BY;
        if (s == "HAVING") return TokenType::HAVING;
        if (s == "ORDER") return TokenType::ORDER;
        if (s == "ASC") return TokenType::ASC;
        if (s == "DESC") return TokenType::DESC;
        if (s == "LIMIT") return TokenType::LIMIT;
        if (s == "SUM") return TokenType::SUM;
        if (s == "COUNT") return TokenType::COUNT;
        if (s == "AVG") return TokenType::AVG;
        if (s == "AS") return TokenType::AS;
        if (s == "AND") return TokenType::AND;
        if (s == "OR") return TokenType::OR;
        return TokenType::IDENTIFIER;
    }

    Token current() { return tokens_[pos_]; }
    void advance() { ++pos_; }

    SelectStmt parse_select() {
        SelectStmt stmt;
        expect(TokenType::SELECT);
        stmt.select_list = parse_select_list();
        expect(TokenType::FROM);
        stmt.from_table = expect(TokenType::IDENTIFIER).value;
        while (current().type == TokenType::INNER) {
            advance();
            expect(TokenType::JOIN);
            std::string join_table = expect(TokenType::IDENTIFIER).value;
            expect(TokenType::ON);
            auto on_condition = parse_predicate();
            stmt.joins.push_back({join_table, std::move(on_condition)});
        }
        if (current().type == TokenType::WHERE) {
            advance();
            stmt.where_clause = parse_expr();
        }
        if (current().type == TokenType::GROUP) {
            advance();
            expect(TokenType::BY);
            stmt.group_by.columns = parse_expr_list();
            if (current().type == TokenType::HAVING) {
                advance();
                stmt.group_by.having = parse_predicate();
            }
        }
        if (current().type == TokenType::ORDER) {
            advance();
            expect(TokenType::BY);
            stmt.order_by = parse_order_list();
        }
        if (current().type == TokenType::LIMIT) {
            advance();
            stmt.limit = std::stoi(expect(TokenType::NUMBER).value);
        }
        return stmt;
    }

    std::vector<SelectItem> parse_select_list() {
        std::vector<SelectItem> list;
        while (true) {
            if (current().type == TokenType::MUL) {
                advance();
                // * means all columns, but for simplicity, leave empty or handle later
            } else {
                auto expr = parse_expr();
                std::string alias;
                if (current().type == TokenType::AS) {
                    advance();
                    alias = expect(TokenType::IDENTIFIER).value;
                }
                list.push_back({alias, std::move(expr)});
            }
            if (current().type != TokenType::COMMA) break;
            advance();
        }
        return list;
    }

    std::unique_ptr<Expr> parse_expr() {
        return parse_or_expr();
    }

    std::unique_ptr<Expr> parse_predicate() {
        return parse_or_expr();
    }

    std::unique_ptr<Expr> parse_or_expr() {
        auto left = parse_and_expr();
        while (current().type == TokenType::OR) {
            advance();
            auto right = parse_and_expr();
            auto expr = std::make_unique<Expr>();
            expr->type = ExprType::BINARY_OP;
            expr->op = BinaryOp::OR;
            expr->left = std::move(left);
            expr->right = std::move(right);
            left = std::move(expr);
        }
        return left;
    }

    std::unique_ptr<Expr> parse_and_expr() {
        auto left = parse_cmp_expr();
        while (current().type == TokenType::AND) {
            advance();
            auto right = parse_cmp_expr();
            auto expr = std::make_unique<Expr>();
            expr->type = ExprType::BINARY_OP;
            expr->op = BinaryOp::AND;
            expr->left = std::move(left);
            expr->right = std::move(right);
            left = std::move(expr);
        }
        return left;
    }

    std::unique_ptr<Expr> parse_cmp_expr() {
        auto left = parse_add_expr();
        if (is_cmp_op(current().type)) {
            BinaryOp op = token_to_cmp_op(current().type);
            advance();
            auto right = parse_add_expr();
            auto expr = std::make_unique<Expr>();
            expr->type = ExprType::BINARY_OP;
            expr->op = op;
            expr->left = std::move(left);
            expr->right = std::move(right);
            return expr;
        }
        return left;
    }

    std::unique_ptr<Expr> parse_add_expr() {
        auto left = parse_mul_expr();
        while (current().type == TokenType::PLUS || current().type == TokenType::MINUS) {
            BinaryOp op = current().type == TokenType::PLUS ? BinaryOp::ADD : BinaryOp::SUB;
            advance();
            auto right = parse_mul_expr();
            auto expr = std::make_unique<Expr>();
            expr->type = ExprType::BINARY_OP;
            expr->op = op;
            expr->left = std::move(left);
            expr->right = std::move(right);
            left = std::move(expr);
        }
        return left;
    }

    std::unique_ptr<Expr> parse_mul_expr() {
        auto left = parse_factor();
        while (current().type == TokenType::MUL || current().type == TokenType::DIV) {
            BinaryOp op = current().type == TokenType::MUL ? BinaryOp::MUL : BinaryOp::DIV;
            advance();
            auto right = parse_factor();
            auto expr = std::make_unique<Expr>();
            expr->type = ExprType::BINARY_OP;
            expr->op = op;
            expr->left = std::move(left);
            expr->right = std::move(right);
            left = std::move(expr);
        }
        return left;
    }

    std::unique_ptr<Expr> parse_factor() {
        if (current().type == TokenType::LPAREN) {
            advance();
            auto expr = parse_expr();
            expect(TokenType::RPAREN);
            return expr;
        } else {
            return parse_primary();
        }
    }



    std::unique_ptr<Expr> parse_primary() {
        auto token = current();
        advance();
        if (token.type == TokenType::IDENTIFIER || token.type == TokenType::SUM || token.type == TokenType::COUNT || token.type == TokenType::AVG) {
            // Check if this is a function call
            if (current().type == TokenType::LPAREN) {
                advance(); // consume '('
                auto expr = std::make_unique<Expr>();
                expr->type = ExprType::FUNC_CALL;
                expr->func_name = token.value;
                // Parse arguments
                if (current().type != TokenType::RPAREN) {
                    while (true) {
                        expr->args.push_back(parse_expr());
                        if (current().type != TokenType::COMMA) break;
                        advance();
                    }
                }
                expect(TokenType::RPAREN);
                return expr;
            } else {
                // Column reference
                auto expr = std::make_unique<Expr>();
                expr->type = ExprType::COLUMN_REF;
                expr->str_val = token.value;
                return expr;
            }
        } else if (token.type == TokenType::MUL) {
            // Handle * as column reference (for COUNT(*))
            auto expr = std::make_unique<Expr>();
            expr->type = ExprType::COLUMN_REF;
            expr->str_val = "*";
            return expr;
        } else if (token.type == TokenType::NUMBER) {
            auto expr = std::make_unique<Expr>();
            expr->type = ExprType::LITERAL_INT;
            expr->i64_val = std::stoll(token.value);
            return expr;
        } else if (token.type == TokenType::STRING_LITERAL) {
            auto expr = std::make_unique<Expr>();
            expr->type = ExprType::LITERAL_STRING;
            expr->str_val = token.value.substr(1, token.value.size() - 2); // remove quotes
            return expr;
        }
        throw std::runtime_error("Unexpected token in expression");
    }



    std::vector<std::unique_ptr<Expr> > parse_expr_list() {
        std::vector<std::unique_ptr<Expr> > list;
        while (true) {
            list.push_back(parse_expr());
            if (current().type != TokenType::COMMA) break;
            advance();
        }
        return list;
    }

    std::vector<OrderByItem> parse_order_list() {
        std::vector<OrderByItem> list;
        while (true) {
            auto expr = parse_expr();
            bool asc = true;
            if (current().type == TokenType::ASC) {
                advance();
            } else if (current().type == TokenType::DESC) {
                asc = false;
                advance();
            }
            list.push_back({std::move(expr), asc});
            if (current().type != TokenType::COMMA) break;
            advance();
        }
        return list;
    }



    bool is_cmp_op(TokenType t) {
        return t == TokenType::EQ || t == TokenType::NE || t == TokenType::LT || t == TokenType::LE || t == TokenType::GT || t == TokenType::GE;
    }

    BinaryOp token_to_cmp_op(TokenType t) {
        switch (t) {
            case TokenType::EQ: return BinaryOp::EQ;
            case TokenType::NE: return BinaryOp::NE;
            case TokenType::LT: return BinaryOp::LT;
            case TokenType::LE: return BinaryOp::LE;
            case TokenType::GT: return BinaryOp::GT;
            case TokenType::GE: return BinaryOp::GE;
            default: throw std::runtime_error("Not a comparison op");
        }
    }

    Token expect(TokenType type) {
        if (current().type != type) {
            throw std::runtime_error("Expected " + std::to_string(static_cast<int>(type)) + " got " + std::to_string(static_cast<int>(current().type)));
        }
        auto t = current();
        advance();
        return t;
    }
};

SelectStmt parse_sql(const std::string& sql) {
    Parser p(sql);
    return p.parse();
}