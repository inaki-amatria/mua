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

#ifndef MUA_AST_WALKER_H
#define MUA_AST_WALKER_H

#include "mua/AST/TranslationUnit.h"
#include "llvm/Support/Casting.h"

namespace mua::ast {

/// Generic AST Walker using a Visitor with onEnter/onExit callbacks
template <typename Visitor> struct Walker final {
  Walker(Visitor &theVisitor) : TheVisitor{theVisitor} {}

  void walk(const Node &n) {
    if (!TheVisitor.onEnter(n)) {
      return;
    }

    if (const auto *expr{llvm::dyn_cast<Expr>(&n)}) {
      if (!TheVisitor.onEnter(*expr)) {
        return;
      }
    }
    if (const auto *stmt{llvm::dyn_cast<Stmt>(&n)}) {
      if (!TheVisitor.onEnter(*stmt)) {
        return;
      }
    }
    if (const auto *decl{llvm::dyn_cast<Decl>(&n)}) {
      if (!TheVisitor.onEnter(*decl)) {
        return;
      }
    }

    switch (n.getKind()) {
    case Node::Kind::NumberExpr:
      walkChildren(static_cast<const NumberExpr &>(n));
      break;
    case Node::Kind::IdentifierExpr:
      walkChildren(static_cast<const IdentifierExpr &>(n));
      break;
    case Node::Kind::CallExpr:
      walkChildren(static_cast<const CallExpr &>(n));
      break;
    case Node::Kind::BinaryExpr:
      walkChildren(static_cast<const BinaryExpr &>(n));
      break;
    case Node::Kind::ExprStmt:
      walkChildren(static_cast<const ExprStmt &>(n));
      break;
    case Node::Kind::ReturnStmt:
      walkChildren(static_cast<const ReturnStmt &>(n));
      break;
    case Node::Kind::CompoundStmt:
      walkChildren(static_cast<const CompoundStmt &>(n));
      break;
    case Node::Kind::ParamDecl:
      walkChildren(static_cast<const ParamDecl &>(n));
      break;
    case Node::Kind::FunctionDecl:
      walkChildren(static_cast<const FunctionDecl &>(n));
      break;
    case Node::Kind::TranslationUnit:
      walkChildren(static_cast<const TranslationUnit &>(n));
      break;
    }

    if (const auto *expr{llvm::dyn_cast<Expr>(&n)}) {
      TheVisitor.onExit(*expr);
    }
    if (const auto *stmt{llvm::dyn_cast<Stmt>(&n)}) {
      TheVisitor.onExit(*stmt);
    }
    if (const auto *decl{llvm::dyn_cast<Decl>(&n)}) {
      TheVisitor.onExit(*decl);
    }

    TheVisitor.onExit(n);
  }

private:
  void walkChildren(const NumberExpr &ne) {
    if (TheVisitor.onEnter(ne)) {
      TheVisitor.onExit(ne);
    }
  }

  void walkChildren(const IdentifierExpr &id) {
    if (TheVisitor.onEnter(id)) {
      TheVisitor.onExit(id);
    }
  }

  void walkChildren(const CallExpr &call) {
    if (TheVisitor.onEnter(call)) {
      for (const ExprPtr &arg : call.getArgs()) {
        walk(*arg);
      }
      TheVisitor.onExit(call);
    }
  }

  void walkChildren(const BinaryExpr &bin) {
    if (TheVisitor.onEnter(bin)) {
      walk(*bin.getLHS());
      walk(*bin.getRHS());
      TheVisitor.onExit(bin);
    }
  }

  void walkChildren(const ExprStmt &es) {
    if (TheVisitor.onEnter(es)) {
      walk(*es.getExpr());
      TheVisitor.onExit(es);
    }
  }

  void walkChildren(const ReturnStmt &rs) {
    if (TheVisitor.onEnter(rs)) {
      walk(*rs.getValue());
      TheVisitor.onExit(rs);
    }
  }

  void walkChildren(const CompoundStmt &cs) {
    if (TheVisitor.onEnter(cs)) {
      for (const StmtPtr &stmt : cs.getStmts()) {
        walk(*stmt);
      }
      TheVisitor.onExit(cs);
    }
  }

  void walkChildren(const ParamDecl &pd) {
    if (TheVisitor.onEnter(pd)) {
      TheVisitor.onExit(pd);
    }
  }

  void walkChildren(const FunctionDecl &fn) {
    if (TheVisitor.onEnter(fn)) {
      for (const ParamDeclPtr &pd : fn.getParams()) {
        walk(*pd);
      }
      walk(*fn.getBody());
      TheVisitor.onExit(fn);
    }
  }

  void walkChildren(const TranslationUnit &tu) {
    if (TheVisitor.onEnter(tu)) {
      for (const FunctionDeclPtr &fn : tu.getFNs()) {
        walk(*fn);
      }
      TheVisitor.onExit(tu);
    }
  }

  Visitor &TheVisitor;
};

/// Convenience function to walk a Node with a Visitor
template <typename NodeTy, typename Visitor>
void Walk(const NodeTy &n, Visitor &v) {
  return Walker<Visitor>{v}.walk(n);
}

} // namespace mua::ast

#endif // MUA_AST_WALKER_H
