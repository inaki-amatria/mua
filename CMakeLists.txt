cmake_minimum_required(VERSION 3.20.0)

project(mua)
set(LLVM_SUBPROJECT_TITLE "mua")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
  add_compile_options(/WX)
elseif(LLVM_COMPILER_IS_GCC_COMPATIBLE)
  add_compile_options(-Werror)
endif()

set(MUA_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(MUA_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR})

include_directories(${MUA_SOURCE_DIR}/include ${MUA_BINARY_DIR}/include)

add_subdirectory(lib)
add_subdirectory(test)
add_subdirectory(tools)
