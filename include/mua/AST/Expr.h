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

#ifndef MUA_AST_EXPR_H
#define MUA_AST_EXPR_H

#include "mua/AST/Node.h"
#include "llvm/ADT/ArrayRef.h"

namespace mua::ast {

/// Base class for all Expressions
class Expr : public Node {
protected:
  Expr(Kind kind, source::Range range) : Node{kind, range} {}

public:
  static bool classof(const Node *n) {
    return n->getKind() >= Kind::FirstExpr && n->getKind() <= Kind::LastExpr;
  }
};

using ExprPtr = std::unique_ptr<Expr>;

/// Expression representing a numeric literal
struct NumberExpr final : public Expr {
  NumberExpr(double value, source::Range range)
      : Expr{Kind::NumberExpr, range}, Value{value} {}

  double getValue() const { return Value; }

  static bool classof(const Node *n) {
    return n->getKind() == Kind::NumberExpr;
  }

private:
  double Value;
};

/// Expression representing an identifier
struct IdentifierExpr final : public Expr {
  IdentifierExpr(source::Text name)
      : Expr{Kind::IdentifierExpr, name.getRange()}, Name{name} {}

  source::Text getName() const { return Name; }

  static bool classof(const Node *n) {
    return n->getKind() == Kind::IdentifierExpr;
  }

private:
  source::Text Name;
};

/// Expression representing a function call
struct CallExpr final : public Expr {
  CallExpr(source::Text callee, std::vector<ExprPtr> args, source::Range range)
      : Expr{Kind::CallExpr, range}, Callee{callee}, Args{std::move(args)} {}

  source::Text getCallee() const { return Callee; }
  llvm::ArrayRef<ExprPtr> getArgs() const { return Args; }

  static bool classof(const Node *n) { return n->getKind() == Kind::CallExpr; }

private:
  source::Text Callee;
  std::vector<ExprPtr> Args;
};

/// Expression representing a binary operation
struct BinaryExpr final : public Expr {
  enum class Op {
    Assign,
    Add,
    Sub,
    Mul,
    Div,
  };

  BinaryExpr(Op op, ExprPtr lhs, ExprPtr rhs, source::Range range)
      : Expr{Kind::BinaryExpr, range}, TheOp{op}, LHS{std::move(lhs)},
        RHS{std::move(rhs)} {}

  Op getOp() const { return TheOp; }
  const Expr *getLHS() const { return LHS.get(); }
  const Expr *getRHS() const { return RHS.get(); }

  static bool classof(const Node *n) {
    return n->getKind() == Kind::BinaryExpr;
  }

private:
  Op TheOp;
  ExprPtr LHS;
  ExprPtr RHS;
};

llvm::raw_ostream &operator<<(llvm::raw_ostream &, BinaryExpr::Op);

} // namespace mua::ast

#endif // MUA_AST_EXPR_H
