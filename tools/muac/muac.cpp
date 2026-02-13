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
#include "mua/Lower/IRUnit.h"
#include "mua/Lower/Lower.h"
#include "mua/Parser/Parser.h"
#include "mua/Sema/Sema.h"
#include "mua/Sema/Symbol.h"
#include "mua/Source/File.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/InitLLVM.h"

static llvm::cl::opt<std::string> InputFilename{
    llvm::cl::Positional, llvm::cl::desc{"<input mua file>"},
    llvm::cl::init("-"), llvm::cl::value_desc{"filename"}};

namespace {
enum class Action { None, DumpAST, DumpSema, DumpLLVM };
} // namespace

static llvm::cl::opt<enum Action> EmitAction(
    "emit", llvm::cl::desc{"Select the intermediate representation to emit"},
    llvm::cl::init(Action::None),
    llvm::cl::values(clEnumValN(Action::DumpAST, "ast",
                                "Emit an abstract syntax tree dump")),
    llvm::cl::values(clEnumValN(Action::DumpSema, "sema",
                                "Emit the semantic representation")),
    llvm::cl::values(clEnumValN(Action::DumpLLVM, "llvm",
                                "Emit the LLVM IR module")));

int main(int argc, char *argv[]) {
  llvm::InitLLVM initLLVM{argc, argv};
  if (!llvm::cl::ParseCommandLineOptions(argc, argv, "mua compiler\n",
                                         &llvm::errs())) {
    return 1;
  }

  std::unique_ptr<mua::source::File> file{
      mua::source::File::Open(InputFilename, llvm::errs())};
  if (!file) {
    return 2;
  }

  std::unique_ptr<mua::ast::TranslationUnit> translationUnit{
      mua::parser::Parse(*file, llvm::errs())};
  if (!translationUnit) {
    return 3;
  }
  if (EmitAction == Action::DumpAST) {
    mua::ast::Dump(*translationUnit, llvm::errs());
    return 0;
  }

  std::unique_ptr<mua::sema::Scope> scope{
      mua::sema::Analyze(*translationUnit, llvm::errs())};
  if (!scope) {
    return 4;
  }
  if (EmitAction == Action::DumpSema) {
    mua::sema::Dump(*scope, llvm::errs());
    return 0;
  }

  mua::lower::IRUnit theIRUnit{
      mua::lower::LowerToLLVMIR(*translationUnit, *scope)};
  if (EmitAction == Action::DumpLLVM) {
    mua::lower::Dump(theIRUnit, llvm::errs());
    return 0;
  }

  return 0;
}
