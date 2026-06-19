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

  bool onEnter(const ast::IdentifierExpr &ie) {
    CurrentScope->declare(Symbol::Kind::Var, ie.getName());
    return true;
  }

  bool onEnter(const ast::CallExpr &ce) {
    const Symbol *symbol{CurrentScope->lookup(ce.getCallee())};
    if (!symbol) {
      error(ce.getRange(), "use of undeclared function " + ce.getCallee());
      return false;
    }
    if (symbol->getKind() != Symbol::Kind::Function) {
      error(ce.getRange(),
            "called object " + ce.getCallee() + " is not a function");
      note(symbol->getName().getRange(), "previous definition is here");
      return false;
    }
    if (ce.getArgs().size() !=
        symbol->getScope()->getSymbols(Symbol::Kind::Param).size()) {
      error(ce.getRange(), "call to function " + ce.getCallee() +
                               " with incorrect number of arguments");
      return false;
    }
    return llvm::all_of(ce.getArgs(), [&](const ast::ExprPtr &arg) {
      return checkValueExpr(*arg);
    });
  }

  bool onEnter(const ast::BinaryExpr &be) {
    if (be.getOp() == ast::BinaryExpr::Op::Assign) {
      const auto *ie{llvm::dyn_cast<ast::IdentifierExpr>(be.getLHS())};
      if (!ie) {
        error(be.getLHS()->getRange(), "expression is not assignable");
        return false;
      }
    }
    return checkValueExpr(*be.getLHS()) && checkValueExpr(*be.getRHS());
  }

  bool onEnter(const ast::UnaryExpr &ue) {
    return checkValueExpr(*ue.getOperand());
  }

  bool onEnter(const ast::ExprStmt &es) {
    return checkValueExpr(*es.getExpr());
  }

  bool onEnter(const ast::ReturnStmt &rs) {
    return checkValueExpr(*rs.getValue());
  }

  bool onEnter(const ast::IfStmt &is) {
    return checkValueExpr(*is.getCondition());
  }

  bool onEnter(const ast::WhileStmt &ws) {
    return checkValueExpr(*ws.getCondition());
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
    if (!checkControlFlow(*fn.getBody())) {
      error(fn.getRange(), "function " + fn.getName() + " must return a value");
    }
    CurrentScope = CurrentScope->getParent();
  }

  std::unique_ptr<Scope> takeGlobalScope() {
    return Error ? nullptr : std::move(GlobalScope);
  }

private:
  bool checkValueExpr(const ast::Expr &expr) {
    const auto *ie{llvm::dyn_cast<ast::IdentifierExpr>(&expr)};
    if (!ie) {
      return true;
    }
    const Symbol *symbol{CurrentScope->lookup(ie->getName())};
    if (!symbol || symbol->getKind() != Symbol::Kind::Function) {
      return true;
    }
    error(ie->getRange(), "invalid use of function " + ie->getName());
    note(symbol->getName().getRange(), "function declared here");
    return false;
  }

  /// Analyze control flow within a CompoundStmt. Reports unreachable code
  /// after terminal statements. Returns true if the last statement is terminal
  /// (guarantees a return in all paths)
  bool checkControlFlow(const ast::CompoundStmt &cs) {
    bool terminal{false};
    for (const ast::StmtPtr &stmt : cs.getStmts()) {
      if (terminal) {
        error(stmt->getRange(), "unreachable statement");
        continue;
      }
      if (llvm::isa<ast::ReturnStmt>(stmt.get())) {
        terminal = true;
      }
      if (const auto *is{llvm::dyn_cast<ast::IfStmt>(stmt.get())}) {
        bool bodyTerminal{checkControlFlow(*is->getIfBody())};
        if (const ast::CompoundStmt *elseBody{is->getElseBody()}) {
          bool elseTerminal{checkControlFlow(*elseBody)};
          if (bodyTerminal && elseTerminal) {
            terminal = true;
          }
        }
      }
      if (const auto *ws{llvm::dyn_cast<ast::WhileStmt>(stmt.get())}) {
        checkControlFlow(*ws->getBody());
      }
    }
    return terminal;
  }

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
  for (const Symbol *symbol : scope.getSymbols()) {
    printIndent();
    os << *symbol << '\n';
    if (const Scope *scope{symbol->getScope()}) {
      DumpScope(*scope, os, indent + 1);
    }
  }
}

void mua::sema::Dump(const Scope &scope, llvm::raw_ostream &os) {
  DumpScope(scope, os);
}
