// MIT License
//
// Copyright (c) 2026-onwards Iñaki Amatria-Barral
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include "mua/Parser/Parser.h"

#include "mua/AST/TranslationUnit.h"
#include "mua/Source/File.h"
#include "mua/Support/ErrorHandling.h"
#include "llvm/Support/raw_ostream.h"

#include "Lexer.h"

using namespace mua;
using namespace mua::parser;

static Token ToToken(ast::UnaryExpr::Op op) {
  switch (op) {
  case ast::UnaryExpr::Op::Neg:
    return Token::Minus;
  case ast::UnaryExpr::Op::Not:
    return Token::Not;
  }
  MUA_COVERS_ALL_CASES;
}

namespace {

struct Expected final {
  enum class Kind {
    Token,
    Expr,
  };

  Expected(Token token) : TheKind{Kind::Token}, TheToken{token} {}
  Expected(Kind kind) : TheKind{kind}, TheToken{Token::Invalid} {
    assert(TheKind != Kind::Token);
  }
  friend llvm::raw_ostream &operator<<(llvm::raw_ostream &, Expected);

private:
  Kind TheKind;
  Token TheToken;
};

llvm::raw_ostream &operator<<(llvm::raw_ostream &os, Expected expected) {
  switch (expected.TheKind) {
  case Expected::Kind::Token:
    return os << expected.TheToken;
  case Expected::Kind::Expr:
    return os << "expression";
  }
  MUA_COVERS_ALL_CASES;
}

struct BinaryExprOp final {
  static std::optional<BinaryExprOp> Create(Token token) {
    switch (token) {
    case Token::Equal:
      return BinaryExprOp{ast::BinaryExpr::Op::Assign};
    case Token::Plus:
      return BinaryExprOp{ast::BinaryExpr::Op::Add};
    case Token::Minus:
      return BinaryExprOp{ast::BinaryExpr::Op::Sub};
    case Token::Star:
      return BinaryExprOp{ast::BinaryExpr::Op::Mul};
    case Token::Slash:
      return BinaryExprOp{ast::BinaryExpr::Op::Div};
    case Token::EqualEqual:
      return BinaryExprOp{ast::BinaryExpr::Op::Eq};
    case Token::NotEqual:
      return BinaryExprOp{ast::BinaryExpr::Op::NotEq};
    case Token::LessOrEqual:
      return BinaryExprOp{ast::BinaryExpr::Op::Le};
    case Token::GreaterOrEqual:
      return BinaryExprOp{ast::BinaryExpr::Op::Ge};
    case Token::LessThan:
      return BinaryExprOp{ast::BinaryExpr::Op::Lt};
    case Token::GreaterThan:
      return BinaryExprOp{ast::BinaryExpr::Op::Gt};
    case Token::And:
      return BinaryExprOp{ast::BinaryExpr::Op::And};
    case Token::Or:
      return BinaryExprOp{ast::BinaryExpr::Op::Or};
    default:
      return std::nullopt;
    }
  }

  ast::BinaryExpr::Op getOp() const { return Op; }

  int getPrecedence() const {
    switch (Op) {
    case ast::BinaryExpr::Op::Assign:
      return 10;
    case ast::BinaryExpr::Op::Or:
      return 20;
    case ast::BinaryExpr::Op::And:
      return 30;
    case ast::BinaryExpr::Op::Eq:
    case ast::BinaryExpr::Op::NotEq:
    case ast::BinaryExpr::Op::Le:
    case ast::BinaryExpr::Op::Ge:
    case ast::BinaryExpr::Op::Lt:
    case ast::BinaryExpr::Op::Gt:
      return 40;
    case ast::BinaryExpr::Op::Add:
    case ast::BinaryExpr::Op::Sub:
      return 50;
    case ast::BinaryExpr::Op::Mul:
    case ast::BinaryExpr::Op::Div:
      return 60;
    }
    MUA_COVERS_ALL_CASES;
  }

  bool isRightAssociative() const { return Op == ast::BinaryExpr::Op::Assign; }

private:
  BinaryExprOp(ast::BinaryExpr::Op op) : Op{op} {}

  ast::BinaryExpr::Op Op;
};

struct Parser final {
  Parser(const source::File &file, llvm::raw_ostream &os)
      : OS{os}, TheLexer{file} {}

  std::unique_ptr<ast::TranslationUnit> parseTranslationUnit() {
    source::Position begin{TheLexer.getRange().getBegin()};
    TheLexer.next(); // Prime the lexer

    std::vector<ast::FunctionDeclPtr> fns;
    while (TheLexer.getCurrent() != Token::EndOfFile) {
      ast::FunctionDeclPtr fn{parseFunctionDecl("at top level")};
      if (!fn) {
        return nullptr;
      }
      fns.push_back(std::move(fn));
    }
    source::Position end{TheLexer.getRange().getEnd()};
    TheLexer.consume(Token::EndOfFile);

    return std::make_unique<ast::TranslationUnit>(std::move(fns),
                                                  source::Range{begin, end});
  }

private:
  ast::ExprPtr parseExpr(llvm::StringRef context) {
    return parseBinaryExpr(/*minPrec=*/0, context);
  }

  ast::ExprPtr parsePrimaryExpr(llvm::StringRef context) {
    switch (TheLexer.getCurrent()) {
    case Token::Number:
      return parseNumberExpr(context);
    case Token::Identifier:
      return parseIdentifierOrCallExpr();
    case Token::Minus:
      return parseUnaryExpr(ast::UnaryExpr::Op::Neg, context);
    case Token::Not:
      return parseUnaryExpr(ast::UnaryExpr::Op::Not, context);
    default:
      return error<ast::Expr>(Expected::Kind::Expr, context);
    }
  }

  ast::ExprPtr parseNumberExpr(llvm::StringRef context) {
    source::Range range{TheLexer.getRange()};
    double value;
    if (llvm::StringRef{source::Text{range}}.getAsDouble(value)) {
      return error<ast::Expr>(Token::Number, context);
    }
    TheLexer.consume(Token::Number);
    return std::make_unique<ast::NumberExpr>(value, range);
  }

  ast::ExprPtr parseIdentifierOrCallExpr() {
    source::Text name{TheLexer.getRange()};
    TheLexer.consume(Token::Identifier);

    if (TheLexer.getCurrent() != Token::LParen) {
      return std::make_unique<ast::IdentifierExpr>(name);
    }
    TheLexer.consume(Token::LParen);

    std::vector<ast::ExprPtr> args;
    while (TheLexer.getCurrent() != Token::RParen) {
      ast::ExprPtr arg{parseExpr("in call argument list")};
      if (!arg) {
        return nullptr;
      }
      args.push_back(std::move(arg));

      if (TheLexer.getCurrent() != Token::Comma) {
        break;
      }
      TheLexer.consume(Token::Comma);
    }

    if (TheLexer.getCurrent() != Token::RParen) {
      return error<ast::Expr>(Token::RParen, "after call argument list");
    }
    source::Position end{TheLexer.getRange().getEnd()};
    TheLexer.consume(Token::RParen);

    return std::make_unique<ast::CallExpr>(
        name, std::move(args), source::Range{name.getRange().getBegin(), end});
  }

  ast::ExprPtr parseUnaryExpr(ast::UnaryExpr::Op op, llvm::StringRef context) {
    source::Position begin{TheLexer.getRange().getBegin()};
    TheLexer.consume(ToToken(op));

    ast::ExprPtr operand{parsePrimaryExpr(context)};
    if (!operand) {
      return nullptr;
    }
    source::Position end{operand->getRange().getEnd()};

    return std::make_unique<ast::UnaryExpr>(op, std::move(operand),
                                            source::Range{begin, end});
  }

  ast::ExprPtr parseBinaryExpr(int minPrec, llvm::StringRef context) {
    ast::ExprPtr lhs{parsePrimaryExpr(context)};
    if (!lhs) {
      return nullptr;
    }

    while (true) {
      Token token{TheLexer.getCurrent()};
      std::optional<BinaryExprOp> binaryExprOp{BinaryExprOp::Create(token)};
      if (!binaryExprOp || binaryExprOp->getPrecedence() < minPrec) {
        break;
      }
      TheLexer.consume(token);

      int nextMinPrec{binaryExprOp->getPrecedence()};
      if (binaryExprOp->isRightAssociative()) {
        ++nextMinPrec;
      }

      ast::ExprPtr rhs{parseBinaryExpr(
          nextMinPrec, "in the right-hand side of a binary expression")};
      if (!rhs) {
        return nullptr;
      }

      source::Range range{lhs->getRange().getBegin(), rhs->getRange().getEnd()};

      lhs = std::make_unique<ast::BinaryExpr>(
          binaryExprOp->getOp(), std::move(lhs), std::move(rhs), range);
    }

    return lhs;
  }

  ast::StmtPtr parseStmt(llvm::StringRef context) {
    switch (TheLexer.getCurrent()) {
    case Token::Return:
      return parseReturnStmt();
    case Token::If:
      return parseIfStmt();
    case Token::While:
      return parseWhileStmt();
    default:
      return parseExprStmt(context);
    }
  }

  ast::StmtPtr parseExprStmt(llvm::StringRef context) {
    ast::ExprPtr expr{parseExpr(context)};
    if (!expr) {
      return nullptr;
    }
    return std::make_unique<ast::ExprStmt>(std::move(expr));
  }

  ast::StmtPtr parseReturnStmt() {
    source::Position begin{TheLexer.getRange().getBegin()};
    TheLexer.consume(Token::Return);

    ast::ExprPtr value{parseExpr("after return")};
    if (!value) {
      return nullptr;
    }
    source::Position end{value->getRange().getEnd()};

    return std::make_unique<ast::ReturnStmt>(std::move(value),
                                             source::Range{begin, end});
  }

  ast::CompoundStmtPtr
  parseCompoundStmt(llvm::StringRef context,
                    std::initializer_list<Token> terminators) {
    source::Position begin{TheLexer.getRange().getBegin()};

    auto isTerminator{[&](Token token) {
      for (Token terminator : terminators) {
        if (token == terminator) {
          return true;
        }
      }
      return false;
    }};

    std::vector<ast::StmtPtr> stmts;
    while (!isTerminator(TheLexer.getCurrent())) {
      ast::StmtPtr stmt{parseStmt(context)};
      if (!stmt) {
        return nullptr;
      }
      stmts.push_back(std::move(stmt));
    }
    source::Position end{TheLexer.getRange().getBegin()};

    return std::make_unique<ast::CompoundStmt>(std::move(stmts),
                                               source::Range{begin, end});
  }

  ast::StmtPtr parseIfStmt() {
    source::Position begin{TheLexer.getRange().getBegin()};
    bool isOuter{TheLexer.getCurrent() == Token::If};
    TheLexer.consume(isOuter ? Token::If : Token::ElseIf);

    ast::ExprPtr condition{parseExpr("after if")};
    if (!condition) {
      return nullptr;
    }

    if (TheLexer.getCurrent() != Token::Then) {
      return error<ast::IfStmt>(Token::Then, "after condition");
    }
    TheLexer.consume(Token::Then);

    ast::CompoundStmtPtr ifBody{parseCompoundStmt(
        isOuter ? "in if body" : "in elseif body",
        /*terminators=*/{Token::End, Token::Else, Token::ElseIf})};
    if (!ifBody) {
      return nullptr;
    }

    ast::CompoundStmtPtr elseBody;
    if (TheLexer.getCurrent() == Token::ElseIf) {
      ast::StmtPtr elseIf{parseIfStmt()};
      if (!elseIf) {
        return nullptr;
      }
      source::Position elseBegin{elseIf->getRange().getBegin()};
      source::Position elseEnd{elseIf->getRange().getEnd()};
      std::vector<ast::StmtPtr> stmts;
      stmts.push_back(std::move(elseIf));
      elseBody = std::make_unique<ast::CompoundStmt>(
          std::move(stmts), source::Range{elseBegin, elseEnd});
    } else if (TheLexer.getCurrent() == Token::Else) {
      TheLexer.consume(Token::Else);
      elseBody =
          parseCompoundStmt("in else body", /*terminators=*/{Token::End});
      if (!elseBody) {
        return nullptr;
      }
    }

    source::Position end{isOuter ? TheLexer.getRange().getEnd()
                                 : TheLexer.getRange().getBegin()};
    if (isOuter) {
      TheLexer.consume(Token::End);
    }

    return std::make_unique<ast::IfStmt>(std::move(condition),
                                         std::move(ifBody), std::move(elseBody),
                                         source::Range{begin, end});
  }

  ast::StmtPtr parseWhileStmt() {
    source::Position begin{TheLexer.getRange().getBegin()};
    TheLexer.consume(Token::While);

    ast::ExprPtr condition{parseExpr("after while")};
    if (!condition) {
      return nullptr;
    }

    if (TheLexer.getCurrent() != Token::Do) {
      return error<ast::WhileStmt>(Token::Do, "after condition");
    }
    TheLexer.consume(Token::Do);

    ast::CompoundStmtPtr body{
        parseCompoundStmt("in while body", /*terminators=*/{Token::End})};
    if (!body) {
      return nullptr;
    }
    source::Position end{TheLexer.getRange().getEnd()};
    TheLexer.consume(Token::End);

    return std::make_unique<ast::WhileStmt>(
        std::move(condition), std::move(body), source::Range{begin, end});
  }

  ast::FunctionDeclPtr parseFunctionDecl(llvm::StringRef context) {
    if (TheLexer.getCurrent() != Token::Function) {
      return error<ast::FunctionDecl>(Token::Function, context);
    }
    source::Position begin{TheLexer.getRange().getBegin()};
    TheLexer.consume(Token::Function);

    if (TheLexer.getCurrent() != Token::Identifier) {
      return error<ast::FunctionDecl>(Token::Identifier, "after function");
    }
    source::Text name{TheLexer.getRange()};
    TheLexer.consume(Token::Identifier);

    if (TheLexer.getCurrent() != Token::LParen) {
      return error<ast::FunctionDecl>(Token::LParen,
                                      "after function identifier");
    }
    TheLexer.consume(Token::LParen);

    std::vector<ast::ParamDeclPtr> params;
    while (TheLexer.getCurrent() != Token::RParen) {
      if (TheLexer.getCurrent() != Token::Identifier) {
        return error<ast::FunctionDecl>(Token::Identifier,
                                        "in function parameter list");
      }
      source::Text name{TheLexer.getRange()};
      TheLexer.consume(Token::Identifier);

      params.push_back(std::make_unique<ast::ParamDecl>(name));

      if (TheLexer.getCurrent() != Token::Comma) {
        break;
      }
      TheLexer.consume(Token::Comma);
    }

    if (TheLexer.getCurrent() != Token::RParen) {
      return error<ast::FunctionDecl>(Token::RParen,
                                      "after function parameter list");
    }
    TheLexer.consume(Token::RParen);

    ast::CompoundStmtPtr body{
        parseCompoundStmt("in function body", /*terminators=*/{Token::End})};
    if (!body) {
      return nullptr;
    }
    source::Position end{TheLexer.getRange().getEnd()};
    TheLexer.consume(Token::End);

    return std::make_unique<ast::FunctionDecl>(
        name, std::move(params), std::move(body), source::Range{begin, end});
  }

  template <typename T>
  std::unique_ptr<T> error(Expected expected, llvm::StringRef context) {
    source::Range range{TheLexer.getRange()};
    OS << "error: expected " << expected << ' ' << context << '\n';
    range.getFile()->print(range, OS);
    OS << '\n';
    return nullptr;
  }

  llvm::raw_ostream &OS;
  Lexer TheLexer;
};

} // namespace

std::unique_ptr<ast::TranslationUnit>
mua::parser::Parse(const source::File &file, llvm::raw_ostream &os) {
  return Parser{file, os}.parseTranslationUnit();
}
