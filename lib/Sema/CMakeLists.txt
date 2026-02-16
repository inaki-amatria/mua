set(LLVM_LINK_COMPONENTS Support)
llvm_add_library(muaSema Sema.cpp Symbol.cpp)
target_link_libraries(muaSema PUBLIC muaSource)
