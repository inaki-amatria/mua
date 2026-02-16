set(LLVM_LINK_COMPONENTS Support)
llvm_add_library(muaParser Lexer.cpp Parser.cpp Token.cpp)
target_link_libraries(muaParser PUBLIC muaSource)
