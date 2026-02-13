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

#include "mua/AST/TranslationUnit.h"

#include "mua/AST/Walker.h"
#include "llvm/Support/raw_ostream.h"

using namespace mua;
using namespace mua::ast;

namespace {

struct DumpVisitor final {
  DumpVisitor(llvm::raw_ostream &os) : OS{os} {}

  template <typename T> bool onEnter(const T &) { return true; }
  template <typename T> void onExit(const T &) {}

  bool onEnter(const NumberExpr &ne) {
    printIndent();
    OS << "NumberExpr " << ne.getValue() << " [" << ne.getRange() << "]\n";
    return true;
  }

  bool onEnter(const IdentifierExpr &id) {
    printIndent();
    OS << "IdentifierExpr " << id.getName() << " [" << id.getRange() << "]\n";
    return true;
  }

  bool onEnter(const CallExpr &call) {
    printIndent();
    OS << "CallExpr [" << call.getRange() << "]\n";
    ++Level;
    return true;
  }

  void onExit(const CallExpr &) { --Level; }

  bool onEnter(const BinaryExpr &bin) {
    printIndent();
    OS << "BinaryExpr " << bin.getOp() << " [" << bin.getRange() << "]\n";
    ++Level;
    return true;
  }

  void onExit(const BinaryExpr &) { --Level; }

  bool onEnter(const ExprStmt &es) {
    printIndent();
    OS << "ExprStmt [" << es.getRange() << "]\n";
    ++Level;
    return true;
  }

  void onExit(const ExprStmt &) { --Level; }

  bool onEnter(const ReturnStmt &rs) {
    printIndent();
    OS << "ReturnStmt [" << rs.getRange() << "]\n";
    ++Level;
    return true;
  }

  void onExit(const ReturnStmt &) { --Level; }

  bool onEnter(const CompoundStmt &cs) {
    printIndent();
    OS << "CompoundStmt [" << cs.getRange() << "]\n";
    ++Level;
    return true;
  }

  void onExit(const CompoundStmt &) { --Level; }

  bool onEnter(const ParamDecl &pd) {
    printIndent();
    OS << "ParamDecl " << pd.getName() << " [" << pd.getRange() << "]\n";
    return true;
  }

  bool onEnter(const FunctionDecl &fn) {
    printIndent();
    OS << "FunctionDecl " << fn.getName() << " [" << fn.getRange() << "]\n";
    ++Level;
    return true;
  }

  void onExit(const FunctionDecl &) { --Level; }

  bool onEnter(const TranslationUnit &tu) {
    printIndent();
    OS << "TranslationUnit [" << tu.getRange() << "]\n";
    ++Level;
    return true;
  }

  void onExit(const TranslationUnit &) { --Level; }

private:
  void printIndent() {
    for (unsigned i{0}; i < Level; ++i) {
      OS << "  ";
    }
  }

  llvm::raw_ostream &OS;
  unsigned Level{0};
};

} // namespace

void mua::ast::Dump(const TranslationUnit &tu, llvm::raw_ostream &os) {
  DumpVisitor dumpVisitor{os};
  Walk(tu, dumpVisitor);
}
