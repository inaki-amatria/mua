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

  bool onEnter(const ast::IfStmt &is) {
    llvm::Function *function{IRBuilder.GetInsertBlock()->getParent()};

    llvm::BasicBlock *thenBB{
        llvm::BasicBlock::Create(*LLVMContext, "if.then", function)};
    llvm::BasicBlock *elseBB{nullptr};
    if (is.getElseBody()) {
      elseBB = llvm::BasicBlock::Create(*LLVMContext, "if.else", function);
    }
    llvm::BasicBlock *mergeBB{
        llvm::BasicBlock::Create(*LLVMContext, "if.end", function)};

    llvm::Value *condition{lower(*is.getCondition())};
    IRBuilder.CreateCondBr(isTruthy(condition), thenBB,
                           is.getElseBody() ? elseBB : mergeBB);

    IRBuilder.SetInsertPoint(thenBB);
    ast::Walk(*is.getIfBody(), *this);
    bool thenTerminated{IRBuilder.GetInsertBlock()->hasTerminator()};
    if (!thenTerminated) {
      IRBuilder.CreateBr(mergeBB);
    }

    bool elseTerminated{false};
    if (const ast::CompoundStmt *elseBody{is.getElseBody()}) {
      IRBuilder.SetInsertPoint(elseBB);
      ast::Walk(*elseBody, *this);
      elseTerminated = IRBuilder.GetInsertBlock()->hasTerminator();
      if (!elseTerminated) {
        IRBuilder.CreateBr(mergeBB);
      }
    }

    if (thenTerminated && elseTerminated) {
      mergeBB->eraseFromParent();
    } else {
      IRBuilder.SetInsertPoint(mergeBB);
    }

    return false;
  }

  bool onEnter(const ast::WhileStmt &ws) {
    llvm::Function *function{IRBuilder.GetInsertBlock()->getParent()};

    llvm::BasicBlock *condBB{
        llvm::BasicBlock::Create(*LLVMContext, "while.cond", function)};
    llvm::BasicBlock *bodyBB{
        llvm::BasicBlock::Create(*LLVMContext, "while.body", function)};
    llvm::BasicBlock *mergeBB{
        llvm::BasicBlock::Create(*LLVMContext, "while.end", function)};

    IRBuilder.CreateBr(condBB);

    IRBuilder.SetInsertPoint(condBB);
    llvm::Value *condition{lower(*ws.getCondition())};
    IRBuilder.CreateCondBr(isTruthy(condition), bodyBB, mergeBB);

    IRBuilder.SetInsertPoint(bodyBB);
    ast::Walk(*ws.getBody(), *this);
    if (!IRBuilder.GetInsertBlock()->hasTerminator()) {
      IRBuilder.CreateBr(condBB);
    }

    IRBuilder.SetInsertPoint(mergeBB);

    return false;
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
    case ast::Node::Kind::NumberExpr:
      return lowerNumberExpr(static_cast<const ast::NumberExpr &>(expr));
    case ast::Node::Kind::IdentifierExpr:
      return lowerIdentifierExpr(
          static_cast<const ast::IdentifierExpr &>(expr));
    case ast::Node::Kind::CallExpr:
      return lowerCallExpr(static_cast<const ast::CallExpr &>(expr));
    case ast::Node::Kind::BinaryExpr:
      return lowerBinaryExpr(static_cast<const ast::BinaryExpr &>(expr));
    case ast::Node::Kind::UnaryExpr:
      return lowerUnaryExpr(static_cast<const ast::UnaryExpr &>(expr));
    case ast::Node::Kind::ExprStmt:
    case ast::Node::Kind::ReturnStmt:
    case ast::Node::Kind::CompoundStmt:
    case ast::Node::Kind::IfStmt:
    case ast::Node::Kind::WhileStmt:
    case ast::Node::Kind::ParamDecl:
    case ast::Node::Kind::FunctionDecl:
    case ast::Node::Kind::TranslationUnit:
      break;
    }
    MUA_COVERS_ALL_CASES;
  }

  llvm::Value *lowerNumberExpr(const ast::NumberExpr &ne) {
    return getConstant(ne.getValue());
  }

  llvm::Value *lowerIdentifierExpr(const ast::IdentifierExpr &id) {
    const sema::Symbol *symbol{CurrentScope->lookup(id.getName())};
    return IRBuilder.CreateLoad(IRBuilder.getDoubleTy(),
                                SymbolToValue.at(symbol), symbol->getName());
  }

  llvm::Value *lowerCallExpr(const ast::CallExpr &call) {
    llvm::Function *function{Module->getFunction(call.getCallee())};
    std::vector<llvm::Value *> args;
    for (const ast::ExprPtr &arg : call.getArgs()) {
      args.push_back(lower(*arg));
    }
    return IRBuilder.CreateCall(function, args);
  }

  llvm::Value *lowerBinaryExpr(const ast::BinaryExpr &bin) {
    switch (bin.getOp()) {
    case ast::BinaryExpr::Op::Assign: {
      const auto &id{static_cast<const ast::IdentifierExpr &>(*bin.getLHS())};
      const sema::Symbol *symbol{CurrentScope->lookup(id.getName())};
      llvm::Value *rhs{lower(*bin.getRHS())};
      IRBuilder.CreateStore(rhs, SymbolToValue.at(symbol));
      return rhs;
    }
    case ast::BinaryExpr::Op::Add:
      return IRBuilder.CreateFAdd(lower(*bin.getLHS()), lower(*bin.getRHS()));
    case ast::BinaryExpr::Op::Sub:
      return IRBuilder.CreateFSub(lower(*bin.getLHS()), lower(*bin.getRHS()));
    case ast::BinaryExpr::Op::Mul:
      return IRBuilder.CreateFMul(lower(*bin.getLHS()), lower(*bin.getRHS()));
    case ast::BinaryExpr::Op::Div:
      return IRBuilder.CreateFDiv(lower(*bin.getLHS()), lower(*bin.getRHS()));
    case ast::BinaryExpr::Op::Eq:
      return lowerComparison(llvm::FCmpInst::FCMP_OEQ, bin);
    case ast::BinaryExpr::Op::NotEq:
      return lowerComparison(llvm::FCmpInst::FCMP_ONE, bin);
    case ast::BinaryExpr::Op::Lt:
      return lowerComparison(llvm::FCmpInst::FCMP_OLT, bin);
    case ast::BinaryExpr::Op::Gt:
      return lowerComparison(llvm::FCmpInst::FCMP_OGT, bin);
    case ast::BinaryExpr::Op::Le:
      return lowerComparison(llvm::FCmpInst::FCMP_OLE, bin);
    case ast::BinaryExpr::Op::Ge:
      return lowerComparison(llvm::FCmpInst::FCMP_OGE, bin);
    case ast::BinaryExpr::Op::And:
    case ast::BinaryExpr::Op::Or:
      bool isAnd{bin.getOp() == ast::BinaryExpr::Op::And};

      llvm::Function *function{IRBuilder.GetInsertBlock()->getParent()};

      llvm::BasicBlock *condBB{IRBuilder.GetInsertBlock()};
      llvm::BasicBlock *rhsBB{llvm::BasicBlock::Create(
          *LLVMContext, isAnd ? "and.rhs" : "or.rhs", function)};
      llvm::BasicBlock *endBB{llvm::BasicBlock::Create(
          *LLVMContext, isAnd ? "and.end" : "or.end", function)};

      llvm::Value *lhs{lower(*bin.getLHS())};
      IRBuilder.CreateCondBr(isTruthy(lhs), isAnd ? rhsBB : endBB,
                             isAnd ? endBB : rhsBB);

      IRBuilder.SetInsertPoint(rhsBB);
      llvm::Value *rhs{lower(*bin.getRHS())};
      IRBuilder.CreateBr(endBB);

      IRBuilder.SetInsertPoint(endBB);
      llvm::PHINode *phi{IRBuilder.CreatePHI(IRBuilder.getDoubleTy(), 2)};
      phi->addIncoming(lhs, condBB);
      phi->addIncoming(rhs, rhsBB);

      return phi;
    }
    MUA_COVERS_ALL_CASES;
  }

  llvm::Value *lowerUnaryExpr(const ast::UnaryExpr &ue) {
    switch (ue.getOp()) {
    case mua::ast::UnaryExpr::Op::Neg:
      return IRBuilder.CreateFNeg(lower(*ue.getOperand()));
    case mua::ast::UnaryExpr::Op::Not:
      llvm::Value *operand{lower(*ue.getOperand())};
      return IRBuilder.CreateSelect(isTruthy(operand), getConstant(0.0),
                                    getConstant(1.0));
    }
    MUA_COVERS_ALL_CASES;
  }

  llvm::Value *lowerComparison(llvm::FCmpInst::Predicate pred,
                               const ast::BinaryExpr &bin) {
    llvm::Value *cmp{
        IRBuilder.CreateFCmp(pred, lower(*bin.getLHS()), lower(*bin.getRHS()))};
    return IRBuilder.CreateSelect(cmp, getConstant(1.0), getConstant(0.0));
  }

  llvm::Value *isTruthy(llvm::Value *value) {
    return IRBuilder.CreateFCmpONE(value, getConstant(0.0));
  }

  llvm::Constant *getConstant(double value) {
    return llvm::ConstantFP::get(IRBuilder.getDoubleTy(), value);
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
