set(LLVM_LINK_COMPONENTS CodeGen Core MC Support TargetParser native)
llvm_add_library(muaCodeGen CodeGen.cpp)
