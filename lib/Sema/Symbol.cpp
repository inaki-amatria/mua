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

#include "mua/Sema/Symbol.h"

#include "llvm/Support/raw_ostream.h"

using namespace mua;
using namespace mua::sema;

llvm::raw_ostream &mua::sema::operator<<(llvm::raw_ostream &os,
                                         const Symbol &symbol) {
  source::Text name{symbol.getName()};
  os << name << " : ";
  switch (symbol.getKind()) {
  case Symbol::Kind::Param:
    os << "Param";
    break;
  case Symbol::Kind::Function:
    os << "Function";
    break;
  case Symbol::Kind::Var:
    os << "Var";
    break;
  }
  return os << " : " << name.getRange();
}

llvm::raw_ostream &mua::sema::operator<<(llvm::raw_ostream &os,
                                         const Scope &scope) {
  if (const Symbol *symbol{scope.getSymbol()}) {
    os << symbol->getName();
  } else {
    os << "<<unnamed>>";
  }
  os << " : Scope";
  return os;
}
