set(LLVM_LINK_COMPONENTS Core Option Support)
add_llvm_executable(muac muac.cpp)
target_link_libraries(muac
  PRIVATE
    muaAST
    muaCodeGen
    muaLower
    muaParser
    muaSema
    muaSource)
