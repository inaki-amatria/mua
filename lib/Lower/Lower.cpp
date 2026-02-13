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

#include "mua/Lower/Lower.h"

#include "mua/AST/Walker.h"
#include "mua/Lower/IRUnit.h"
#include "mua/Sema/Symbol.h"
#include "mua/Source/File.h"
#include "mua/Support/ErrorHandling.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Verifier.h"

using namespace mua;
using namespace mua::lower;

namespace {

struct LowerToLLVMIRVisitor final {
  LowerToLLVMIRVisitor(const sema::Scope &scope)
      : LLVMContext{std::make_unique<llvm::LLVMContext>()},
        Module{std::make_unique<llvm::Module>("mua module", *LLVMContext)},
        CurrentScope{&scope}, IRBuilder{*LLVMContext} {}

  template <typename T> bool onEnter(const T &) { return true; }
  template <typename T> void onExit(const T &) {}

  bool onEnter(const ast::ExprStmt &es) {
    lower(*es.getExpr());
    return true;
  }

  bool onEnter(const ast::ReturnStmt &rs) {
    IRBuilder.CreateRet(lower(*rs.getValue()));
    return true;
  }

  bool onEnter(const ast::FunctionDecl &fn) {
    const sema::Symbol *symbol{CurrentScope->lookup(fn.getName())};
    const sema::Scope *scope{symbol->getScope()};
    std::vector<const sema::Symbol *> params{
        scope->getSymbols(sema::Symbol::Kind::Param)};
    std::vector<llvm::Type *> paramTys{params.size(), IRBuilder.getDoubleTy()};
    llvm::FunctionType *functionTy{
        llvm::FunctionType::get(IRBuilder.getDoubleTy(), paramTys,
                                /*isVarArg=*/false)};
    llvm::Function *function{
        llvm::Function::Create(functionTy, llvm::Function::ExternalLinkage,
                               symbol->getName(), *Module)};
    llvm::BasicBlock::Create(*LLVMContext,
                             /*Name=*/"", function);
    IRBuilder.SetInsertPoint(&function->getEntryBlock());
    for (auto [symbol, arg] : llvm::zip_equal(params, function->args())) {
      llvm::AllocaInst *alloca{IRBuilder.CreateAlloca(
          IRBuilder.getDoubleTy(), nullptr, symbol->getName())};
      IRBuilder.CreateStore(&arg, alloca);
      SymbolToValue[symbol] = alloca;
    }
    for (const sema::Symbol *symbol :
         scope->getSymbols(sema::Symbol::Kind::Var)) {
      llvm::AllocaInst *alloca{IRBuilder.CreateAlloca(
          IRBuilder.getDoubleTy(), nullptr, symbol->getName())};
      SymbolToValue[symbol] = alloca;
    }
    CurrentScope = scope;
    return true;
  }

  void onExit(const ast::FunctionDecl &fn) {
    assert(!llvm::verifyFunction(*Module->getFunction(fn.getName())));
    CurrentScope = CurrentScope->getParent();
  }

  bool onEnter(const ast::TranslationUnit &tu) {
    Module->setSourceFileName(tu.getRange().getFile()->getFilename());
    return true;
  }

  void onExit(const ast::TranslationUnit &) {
    assert(!llvm::verifyModule(*Module));
  }

  IRUnit takeIRUnit() { return {std::move(LLVMContext), std::move(Module)}; }

private:
  llvm::Value *lower(const ast::Expr &expr) {
    switch (expr.getKind()) {
    case ast::Node::Kind::NumberExpr: {
      const auto &ne{static_cast<const ast::NumberExpr &>(expr)};
      return llvm::ConstantFP::get(IRBuilder.getDoubleTy(), ne.getValue());
    }
    case ast::Node::Kind::IdentifierExpr: {
      const auto &id{static_cast<const ast::IdentifierExpr &>(expr)};
      const sema::Symbol *symbol{CurrentScope->lookup(id.getName())};
      return IRBuilder.CreateLoad(IRBuilder.getDoubleTy(),
                                  SymbolToValue.at(symbol), symbol->getName());
    }
    case ast::Node::Kind::CallExpr: {
      const auto &call{static_cast<const ast::CallExpr &>(expr)};
      const ast::IdentifierExpr *id{call.getCallee()};
      llvm::Function *callee{Module->getFunction(id->getName())};
      std::vector<llvm::Value *> args;
      for (const ast::ExprPtr &arg : call.getArgs()) {
        args.push_back(lower(*arg));
      }
      return IRBuilder.CreateCall(callee, args);
    }
    case ast::Node::Kind::BinaryExpr: {
      const auto &bin{static_cast<const ast::BinaryExpr &>(expr)};
      if (bin.getOp() == ast::BinaryExpr::Op::Assign) {
        const auto &id{static_cast<const ast::IdentifierExpr &>(*bin.getLHS())};
        const sema::Symbol *symbol{CurrentScope->lookup(id.getName())};
        llvm::Value *rhs{lower(*bin.getRHS())};
        llvm::Value *addr{SymbolToValue.at(symbol)};
        IRBuilder.CreateStore(rhs, addr);
        return rhs;
      }
      llvm::Value *lhs{lower(*bin.getLHS())};
      llvm::Value *rhs{lower(*bin.getRHS())};
      switch (bin.getOp()) {
      case ast::BinaryExpr::Op::Add:
        return IRBuilder.CreateFAdd(lhs, rhs);
      case ast::BinaryExpr::Op::Sub:
        return IRBuilder.CreateFSub(lhs, rhs);
      case ast::BinaryExpr::Op::Mul:
        return IRBuilder.CreateFMul(lhs, rhs);
      case ast::BinaryExpr::Op::Div:
        return IRBuilder.CreateFDiv(lhs, rhs);
      case ast::BinaryExpr::Op::Assign:
        break;
      }
      MUA_COVERS_ALL_CASES;
    }
    case ast::Node::Kind::ExprStmt:
    case ast::Node::Kind::ReturnStmt:
    case ast::Node::Kind::CompoundStmt:
    case ast::Node::Kind::ParamDecl:
    case ast::Node::Kind::FunctionDecl:
    case ast::Node::Kind::TranslationUnit:
      break;
    }
    MUA_COVERS_ALL_CASES;
  }

  std::unique_ptr<llvm::LLVMContext> LLVMContext;
  std::unique_ptr<llvm::Module> Module;

  const sema::Scope *CurrentScope;

  llvm::IRBuilder<> IRBuilder;
  llvm::DenseMap<const sema::Symbol *, llvm::Value *> SymbolToValue;
};

} // namespace

IRUnit mua::lower::LowerToLLVMIR(const ast::TranslationUnit &tu,
                                 const sema::Scope &scope) {
  LowerToLLVMIRVisitor lowerToLLVMIRVisitor{scope};
  ast::Walk(tu, lowerToLLVMIRVisitor);
  return lowerToLLVMIRVisitor.takeIRUnit();
}

void mua::lower::Dump(const IRUnit &theIRUnit, llvm::raw_ostream &os) {
  theIRUnit.Module->print(os, /*AAW=*/nullptr);
}
