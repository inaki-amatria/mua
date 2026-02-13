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

#include "mua/Source/Position.h"

#include "mua/Source/File.h"
#include "llvm/Support/raw_ostream.h"

using namespace mua;
using namespace mua::source;

llvm::raw_ostream &mua::source::operator<<(llvm::raw_ostream &os, Range range) {
  const File *file{range.getFile()};

  auto [beginLine, beginColumn]{file->getLineAndColumn(range.getBegin())};
  auto [endLine, endColumn]{file->getLineAndColumn(range.getEnd())};

  os << file->getFilename() << ':';
  if (beginLine == endLine) {
    os << (beginLine + 1) << ':' << (beginColumn + 1) << '-' << (endColumn + 1);
  } else {
    os << (beginLine + 1) << ':' << (beginColumn + 1) << '-' << (endLine + 1)
       << ':' << (endColumn + 1);
  }

  return os;
}

Text::operator llvm::StringRef() const { return getFile()->slice(TheRange); }
Text::operator llvm::Twine() const { return llvm::StringRef{*this}; }

llvm::raw_ostream &mua::source::operator<<(llvm::raw_ostream &os, Text text) {
  return os << llvm::StringRef{text};
}

llvm::Twine mua::source::operator+(const char *lhs, Text rhs) {
  return {lhs, rhs};
}

llvm::Twine mua::source::operator+(Text lhs, const char *rhs) {
  return {lhs, rhs};
}
