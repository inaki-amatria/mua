set(LLVM_LINK_COMPONENTS Core Support)
llvm_add_library(muaLower Lower.cpp)
target_link_libraries(muaLower PUBLIC muaSource)
