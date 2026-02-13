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

#include "Token.h"

#include "mua/Support/ErrorHandling.h"
#include "llvm/Support/raw_ostream.h"

using namespace mua;
using namespace mua::parser;

llvm::raw_ostream &mua::parser::operator<<(llvm::raw_ostream &os, Token token) {
  switch (token) {
#define TOKEN(name, spelling)                                                  \
  case Token::name:                                                            \
    return os << spelling;
#define KEYWORD(name, spelling) TOKEN(name, spelling)
#define PUNCT(name, ch) TOKEN(name, ch)
#include "Token.def"
  }
  MUA_COVERS_ALL_CASES;
}
