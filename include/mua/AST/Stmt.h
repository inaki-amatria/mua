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

#ifndef MUA_AST_STMT_H
#define MUA_AST_STMT_H

#include "mua/AST/Expr.h"

namespace mua::ast {

/// Base class for all Statements
class Stmt : public Node {
protected:
  Stmt(Kind kind, source::Range range) : Node{kind, range} {}

public:
  static bool classof(const Node *n) {
    return n->getKind() >= Kind::FirstStmt && n->getKind() <= Kind::LastStmt;
  }
};

using StmtPtr = std::unique_ptr<Stmt>;

/// Statement wrapping an Expression
struct ExprStmt final : public Stmt {
  ExprStmt(ExprPtr expr)
      : Stmt{Kind::ExprStmt, expr->getRange()}, TheExpr{std::move(expr)} {}

  const Expr *getExpr() const { return TheExpr.get(); }

  static bool classof(const Node *n) { return n->getKind() == Kind::ExprStmt; }

private:
  ExprPtr TheExpr;
};

/// Statement representing a return
struct ReturnStmt final : public Stmt {
  ReturnStmt(ExprPtr value, source::Range range)
      : Stmt{Kind::ReturnStmt, range}, Value{std::move(value)} {}

  const Expr *getValue() const { return Value.get(); }

  static bool classof(const Node *n) {
    return n->getKind() == Kind::ReturnStmt;
  }

private:
  ExprPtr Value;
};

/// Statement representing a compound block of Statements
struct CompoundStmt final : public Stmt {
  CompoundStmt(std::vector<StmtPtr> stmts, source::Range range)
      : Stmt{Kind::CompoundStmt, range}, Stmts{std::move(stmts)} {}

  llvm::ArrayRef<StmtPtr> getStmts() const { return Stmts; }

  static bool classof(const Node *n) {
    return n->getKind() == Kind::CompoundStmt;
  }

private:
  std::vector<StmtPtr> Stmts;
};

using CompoundStmtPtr = std::unique_ptr<CompoundStmt>;

} // namespace mua::ast

#endif // MUA_AST_STMT_H
