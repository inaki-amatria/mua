set(LLVM_LINK_COMPONENTS Support)
llvm_add_library(muaAST Expr.cpp TranslationUnit.cpp)
target_link_libraries(muaAST PUBLIC muaSource)
