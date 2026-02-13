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
    case Token::EndOfFile:
    case Token::Invalid:
    case Token::Identifier:
    case Token::Number:
    case Token::Function:
    case Token::Return:
    case Token::End:
    case Token::Comma:
    case Token::LParen:
    case Token::RParen:
      return std::nullopt;
    }
    MUA_COVERS_ALL_CASES;
  }

  ast::BinaryExpr::Op getOp() const { return Op; }

  int getPrecedence() const {
    switch (Op) {
    case ast::BinaryExpr::Op::Assign:
      return 10;
    case ast::BinaryExpr::Op::Add:
    case ast::BinaryExpr::Op::Sub:
      return 20;
    case ast::BinaryExpr::Op::Mul:
    case ast::BinaryExpr::Op::Div:
      return 30;
    }
    MUA_COVERS_ALL_CASES;
  }

  bool isRightAssociative() const {
    switch (Op) {
    case ast::BinaryExpr::Op::Assign:
      return true;
    case ast::BinaryExpr::Op::Add:
    case ast::BinaryExpr::Op::Sub:
    case ast::BinaryExpr::Op::Mul:
    case ast::BinaryExpr::Op::Div:
      return false;
    }
    MUA_COVERS_ALL_CASES;
  }

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
    case Token::EndOfFile:
    case Token::Invalid:
    case Token::Function:
    case Token::Return:
    case Token::End:
    case Token::Equal:
    case Token::Comma:
    case Token::LParen:
    case Token::RParen:
    case Token::Plus:
    case Token::Minus:
    case Token::Star:
    case Token::Slash:
      return error<ast::Expr>(Expected::Kind::Expr, context);
    }
    MUA_COVERS_ALL_CASES;
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
    auto id{std::make_unique<ast::IdentifierExpr>(name)};
    TheLexer.consume(Token::Identifier);

    if (TheLexer.getCurrent() != Token::LParen) {
      return id;
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
        std::move(id), std::move(args),
        source::Range{name.getRange().getBegin(), end});
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
    case Token::EndOfFile:
    case Token::Invalid:
    case Token::Identifier:
    case Token::Number:
    case Token::Function:
    case Token::End:
    case Token::Equal:
    case Token::Comma:
    case Token::LParen:
    case Token::RParen:
    case Token::Plus:
    case Token::Minus:
    case Token::Star:
    case Token::Slash:
      return parseExprStmt(context);
    }
    MUA_COVERS_ALL_CASES;
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

  ast::CompoundStmtPtr parseCompoundStmt(llvm::StringRef context) {
    source::Position begin{TheLexer.getRange().getBegin()};

    std::vector<ast::StmtPtr> stmts;
    while (TheLexer.getCurrent() != Token::End) {
      ast::StmtPtr stmt{parseStmt(context)};
      if (!stmt) {
        return nullptr;
      }
      stmts.push_back(std::move(stmt));
    }
    source::Position end{TheLexer.getRange().getEnd()};
    TheLexer.consume(Token::End);

    return std::make_unique<ast::CompoundStmt>(std::move(stmts),
                                               source::Range{begin, end});
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

    ast::CompoundStmtPtr body{parseCompoundStmt("in function body")};
    if (!body) {
      return nullptr;
    }
    source::Position end{body->getRange().getEnd()};

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
