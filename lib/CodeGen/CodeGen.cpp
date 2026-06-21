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

#include "mua/CodeGen/CodeGen.h"

#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Module.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/TargetParser/Host.h"

using namespace mua;
using namespace mua::codegen;

bool mua::codegen::EmitObjectFile(llvm::Module &mod, llvm::StringRef outputPath,
                                  llvm::raw_ostream &os) {
  llvm::InitializeAllTargetInfos();
  llvm::InitializeAllTargets();
  llvm::InitializeAllTargetMCs();
  llvm::InitializeAllAsmParsers();
  llvm::InitializeAllAsmPrinters();

  // Detect the host target
  llvm::Triple targetTriple{llvm::sys::getDefaultTargetTriple()};
  mod.setTargetTriple(targetTriple);

  // Look up the target
  std::string errorMsg;
  const llvm::Target *target{
      llvm::TargetRegistry::lookupTarget(targetTriple, errorMsg)};
  if (!target) {
    os << "error: " << errorMsg << '\n';
    return false;
  }

  // Create the target machine
  llvm::TargetOptions targetOptions;
  std::unique_ptr<llvm::TargetMachine> targetMachine{
      target->createTargetMachine(targetTriple, /*CPU=*/"generic",
                                  /*Features=*/"", targetOptions,
                                  llvm::Reloc::PIC_)};

  // Set the data layout
  mod.setDataLayout(targetMachine->createDataLayout());

  // Open the output file
  std::error_code ec;
  llvm::raw_fd_ostream outputFile{outputPath, ec, llvm::sys::fs::OF_None};
  if (ec) {
    os << "error: could not open output file '" << outputPath
       << "': " << ec.message() << '\n';
    return false;
  }

  // ModulePassManager is for IR optimizations
  llvm::PassBuilder pb{targetMachine.get()};

  llvm::LoopAnalysisManager lam;
  llvm::FunctionAnalysisManager fam;
  llvm::CGSCCAnalysisManager cgam;
  llvm::ModuleAnalysisManager mam;

  pb.registerLoopAnalyses(lam);
  pb.registerFunctionAnalyses(fam);
  pb.registerCGSCCAnalyses(cgam);
  pb.registerModuleAnalyses(mam);

  pb.crossRegisterProxies(lam, fam, cgam, mam);

  llvm::ModulePassManager mpm{
      pb.buildPerModuleDefaultPipeline(llvm::OptimizationLevel::O2)};
  mpm.run(mod, mam);

  // PassManager is for codegen
  llvm::legacy::PassManager pm;
  if (targetMachine->addPassesToEmitFile(pm, outputFile, /*DwoOut=*/nullptr,
                                         llvm::CodeGenFileType::ObjectFile)) {
    os << "error: target does not support this file type\n";
    return false;
  }
  pm.run(mod);

  return true;
}
