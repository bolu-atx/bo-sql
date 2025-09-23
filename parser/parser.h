#pragma once

#include <string>
#include <vector>
#include <sstream>
#include <cctype>
#include "ast.h"

enum class TokenType {
    SELECT, FROM, WHERE, INNER, JOIN, ON, GROUP, BY, ORDER, ASC, DESC, LIMIT,
    IDENTIFIER, NUMBER, STRING_LITERAL, COMMA, LPAREN, RPAREN, EQ, NE, LT, LE, GT, GE, PLUS, MINUS, MUL, DIV,
    SUM, COUNT, AVG, AS,
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
        if (s == "ORDER") return TokenType::ORDER;
        if (s == "ASC") return TokenType::ASC;
        if (s == "DESC") return TokenType::DESC;
        if (s == "LIMIT") return TokenType::LIMIT;
        if (s == "SUM") return TokenType::SUM;
        if (s == "COUNT") return TokenType::COUNT;
        if (s == "AVG") return TokenType::AVG;
        if (s == "AS") return TokenType::AS;
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
        if (current().type == TokenType::INNER) {
            advance();
            expect(TokenType::JOIN);
            std::string join_table = expect(TokenType::IDENTIFIER).value;
            expect(TokenType::ON);
            std::string on_cond = parse_expression_string(); // simple
            stmt.joins.emplace_back(join_table, on_cond);
        }
        if (current().type == TokenType::WHERE) {
            advance();
            stmt.where_clause = parse_expr();
        }
        if (current().type == TokenType::GROUP) {
            advance();
            expect(TokenType::BY);
            stmt.group_by.columns = parse_expr_list();
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
        do {
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
        } while (current().type == TokenType::COMMA && advance());
        return list;
    }

    std::unique_ptr<Expr> parse_expr() {
        return parse_binary_expr();
    }

    std::unique_ptr<Expr> parse_binary_expr() {
        auto left = parse_primary();
        if (is_binary_op(current().type)) {
            BinaryOp op = token_to_op(current().type);
            advance();
            auto right = parse_binary_expr();
            auto expr = std::make_unique<Expr>();
            expr->type = ExprType::BINARY_OP;
            expr->op = op;
            expr->left = std::move(left);
            expr->right = std::move(right);
            return expr;
        }
        return left;
    }

    std::unique_ptr<Expr> parse_primary() {
        auto token = current();
        advance();
        if (token.type == TokenType::IDENTIFIER) {
            auto expr = std::make_unique<Expr>();
            expr->type = ExprType::COLUMN_REF;
            expr->value = token.value;
            return expr;
        } else if (token.type == TokenType::NUMBER) {
            auto expr = std::make_unique<Expr>();
            expr->type = ExprType::LITERAL_INT;
            expr->value = std::stoll(token.value);
            return expr;
        } else if (token.type == TokenType::STRING_LITERAL) {
            auto expr = std::make_unique<Expr>();
            expr->type = ExprType::LITERAL_STRING;
            expr->value = token.value.substr(1, token.value.size() - 2); // remove quotes
            return expr;
        }
        throw std::runtime_error("Unexpected token in expression");
    }

    bool is_binary_op(TokenType t) {
        return t == TokenType::EQ || t == TokenType::NE || t == TokenType::LT || t == TokenType::LE || t == TokenType::GT || t == TokenType::GE ||
               t == TokenType::PLUS || t == TokenType::MINUS || t == TokenType::MUL || t == TokenType::DIV;
    }

    BinaryOp token_to_op(TokenType t) {
        switch (t) {
            case TokenType::EQ: return BinaryOp::EQ;
            case TokenType::NE: return BinaryOp::NE;
            case TokenType::LT: return BinaryOp::LT;
            case TokenType::LE: return BinaryOp::LE;
            case TokenType::GT: return BinaryOp::GT;
            case TokenType::GE: return BinaryOp::GE;
            case TokenType::PLUS: return BinaryOp::ADD;
            case TokenType::MINUS: return BinaryOp::SUB;
            case TokenType::MUL: return BinaryOp::MUL;
            case TokenType::DIV: return BinaryOp::DIV;
            default: throw std::runtime_error("Not a binary op");
        }
    }

    std::vector<std::unique_ptr<Expr>> parse_expr_list() {
        std::vector<std::unique_ptr<Expr>> list;
        do {
            list.push_back(parse_expr());
        } while (current().type == TokenType::COMMA && advance());
        return list;
    }

    std::vector<OrderByItem> parse_order_list() {
        std::vector<OrderByItem> list;
        do {
            auto expr = parse_expr();
            bool asc = true;
            if (current().type == TokenType::ASC) {
                advance();
            } else if (current().type == TokenType::DESC) {
                asc = false;
                advance();
            }
            list.push_back({std::move(expr), asc});
        } while (current().type == TokenType::COMMA && advance());
        return list;
    }

    std::string parse_expression_string() {
        // Simple: until end or something, for ON condition
        std::string s;
        while (current().type != TokenType::END && current().type != TokenType::GROUP && current().type != TokenType::ORDER && current().type != TokenType::LIMIT) {
            s += current().value + " ";
            advance();
        }
        return s;
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