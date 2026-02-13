add_llvm_executable(muac muac.cpp)
target_link_libraries(muac
  PRIVATE
    muaAST
    muaLower
    muaParser
    muaSema
    muaSource)
