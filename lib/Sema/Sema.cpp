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

#include "mua/Sema/Sema.h"

#include "mua/AST/Walker.h"
#include "mua/Sema/Symbol.h"
#include "mua/Source/File.h"
#include "mua/Support/ErrorHandling.h"
#include "llvm/Support/raw_ostream.h"

using namespace mua;
using namespace mua::sema;

namespace {

struct AnalyzerVisitor final {
  AnalyzerVisitor(llvm::raw_ostream &os)
      : OS{os}, GlobalScope{std::make_unique<Scope>(/*parent=*/nullptr)},
        CurrentScope{GlobalScope.get()} {}

  template <typename T> bool onEnter(const T &) { return true; }
  template <typename T> void onExit(const T &) {}

  bool onEnter(const ast::IdentifierExpr &id) {
    CurrentScope->declare(Symbol::Kind::Var, id.getName());
    return true;
  }

  bool onEnter(const ast::CallExpr &call) {
    const Symbol *symbol{CurrentScope->lookup(call.getCallee())};
    if (!symbol) {
      error(call.getRange(), "use of undeclared function " + call.getCallee());
      return false;
    }
    if (symbol->getKind() != Symbol::Kind::Function) {
      error(call.getRange(),
            "called object " + call.getCallee() + " is not a function");
      note(symbol->getName().getRange(), "previous definition is here");
      return false;
    }
    if (call.getArgs().size() !=
        symbol->getScope()->getSymbols(Symbol::Kind::Param).size()) {
      error(call.getRange(), "call to function " + call.getCallee() +
                                 " with incorrect number of arguments");
      return false;
    }
    return true;
  }

  bool onEnter(const ast::BinaryExpr &bin) {
    switch (bin.getOp()) {
    case ast::BinaryExpr::Op::Assign: {
      const auto *id{llvm::dyn_cast<ast::IdentifierExpr>(bin.getLHS())};
      if (!id) {
        error(bin.getLHS()->getRange(), "expression is not assignable");
        return false;
      }
      return true;
    }
    case ast::BinaryExpr::Op::Add:
    case ast::BinaryExpr::Op::Sub:
    case ast::BinaryExpr::Op::Mul:
    case ast::BinaryExpr::Op::Div:
      return true;
    }
    MUA_COVERS_ALL_CASES;
  }

  bool onEnter(const ast::ParamDecl &pd) {
    auto [symbol,
          declared]{CurrentScope->declare(Symbol::Kind::Param, pd.getName())};
    if (!declared) {
      error(pd.getRange(), "redefinition of parameter " + pd.getName());
      note(symbol->getName().getRange(), "previous definition is here");
      return false;
    }
    return true;
  }

  bool onEnter(const ast::FunctionDecl &fn) {
    auto [symbol, declared]{
        CurrentScope->declare(Symbol::Kind::Function, fn.getName())};
    if (!declared) {
      error(fn.getRange(), "redefinition of function " + fn.getName());
      note(symbol->getName().getRange(), "previous definition is here");
      return false;
    }
    CurrentScope = symbol->getScope();
    return true;
  }

  void onExit(const ast::FunctionDecl &fn) {
    llvm::ArrayRef<ast::StmtPtr> stmts{fn.getBody()->getStmts()};
    if (stmts.empty()) {
      error(fn.getRange(),
            "function " + fn.getName() + " must end with a return statement");
    } else if (!llvm::isa<ast::ReturnStmt>(stmts.back().get())) {
      error(stmts.back()->getRange(), "last statement of function " +
                                          fn.getName() +
                                          " must be a return statement");
    }
    CurrentScope = CurrentScope->getParent();
  }

  std::unique_ptr<Scope> takeGlobalScope() {
    return Error ? nullptr : std::move(GlobalScope);
  }

private:
  void error(source::Range range, llvm::Twine message) {
    OS << "error: " << message << '\n';
    range.getFile()->print(range, OS);
    OS << '\n';
    Error = true;
  }

  void note(source::Range range, llvm::Twine message) {
    OS << "note: " << message << '\n';
    range.getFile()->print(range, OS);
    OS << '\n';
  }

  llvm::raw_ostream &OS;

  std::unique_ptr<Scope> GlobalScope;
  Scope *CurrentScope;

  bool Error{false};
};

} // namespace

std::unique_ptr<Scope> mua::sema::Analyze(const ast::TranslationUnit &tu,
                                          llvm::raw_ostream &os) {
  AnalyzerVisitor analyzerVisitor{os};
  ast::Walk(tu, analyzerVisitor);
  return analyzerVisitor.takeGlobalScope();
}

static void DumpScope(const Scope &scope, llvm::raw_ostream &os,
                      unsigned indent = 0) {
  auto printIndent{[&]() {
    for (unsigned i{0}; i < indent; ++i) {
      os << "  ";
    }
  }};

  printIndent();
  os << scope << '\n';

  indent++;
  for (const Symbol *sym : scope.getSymbols()) {
    printIndent();
    os << *sym << '\n';
    if (const Scope *scope{sym->getScope()}) {
      DumpScope(*scope, os, indent + 1);
    }
  }
}

void mua::sema::Dump(const Scope &scope, llvm::raw_ostream &os) {
  DumpScope(scope, os);
}
