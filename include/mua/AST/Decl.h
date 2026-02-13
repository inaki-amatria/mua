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

#ifndef MUA_AST_DECL_H
#define MUA_AST_DECL_H

#include "mua/AST/Stmt.h"

namespace mua::ast {

/// Base class for all Declarations
class Decl : public Node {
protected:
  Decl(Kind kind, source::Range range) : Node{kind, range} {}

public:
  static bool classof(const Node *n) {
    return n->getKind() >= Kind::FirstDecl && n->getKind() <= Kind::LastDecl;
  }
};

/// Declaration of a function parameter
struct ParamDecl final : public Decl {
  ParamDecl(source::Text name)
      : Decl{Kind::ParamDecl, name.getRange()}, Name{name} {}

  source::Text getName() const { return Name; }

  static bool classof(const Node *n) { return n->getKind() == Kind::ParamDecl; }

private:
  source::Text Name;
};

using ParamDeclPtr = std::unique_ptr<ParamDecl>;

/// Declaration of a function
struct FunctionDecl final : public Decl {
  FunctionDecl(source::Text name, std::vector<ParamDeclPtr> params,
               CompoundStmtPtr body, source::Range range)
      : Decl{Kind::FunctionDecl, range}, Name{name}, Params{std::move(params)},
        Body{std::move(body)} {}

  source::Text getName() const { return Name; }
  llvm::ArrayRef<ParamDeclPtr> getParams() const { return Params; }
  const CompoundStmt *getBody() const { return Body.get(); }

  static bool classof(const Node *n) {
    return n->getKind() == Kind::FunctionDecl;
  }

private:
  source::Text Name;
  std::vector<ParamDeclPtr> Params;
  CompoundStmtPtr Body;
};

using FunctionDeclPtr = std::unique_ptr<FunctionDecl>;

} // namespace mua::ast

#endif // MUA_AST_DECL_H
