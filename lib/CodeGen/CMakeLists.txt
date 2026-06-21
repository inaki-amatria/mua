set(LLVM_LINK_COMPONENTS
  Analysis
  CodeGen
  Core
  MC
  Passes
  Support
  TargetParser
  native)
llvm_add_library(muaCodeGen CodeGen.cpp)
