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

#ifndef MUA_LOWER_LOWER_H
#define MUA_LOWER_LOWER_H

namespace llvm {
class raw_ostream;
} // namespace llvm

namespace mua::ast {
class TranslationUnit;
} // namespace mua::ast

namespace mua::sema {
struct Scope;
} // namespace mua::sema

namespace mua::lower {

struct IRUnit;

/// Lower the given TranslationUnit into LLVM IR using the provided semantic
/// information in Scope
IRUnit LowerToLLVMIR(const ast::TranslationUnit &, const sema::Scope &);

/// Dump the contents of an IRUnit (the generated LLVM IR) to the given output
/// stream
void Dump(const IRUnit &, llvm::raw_ostream &);

} // namespace mua::lower

#endif // MUA_LOWER_LOWER_H
