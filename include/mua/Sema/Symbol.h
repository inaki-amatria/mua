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

#ifndef MUA_SEMA_SYMBOL_H
#define MUA_SEMA_SYMBOL_H

#include "mua/Source/Position.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/StringMap.h"
#include "llvm/Support/Allocator.h"

namespace mua::sema {

struct Scope;

/// A semantic Symbol
struct Symbol final {
  enum class Kind {
    Param,
    Function,
    Var,
  };

  Symbol(Kind kind, source::Text name)
      : TheKind{kind}, Name{name}, TheScope{nullptr} {}

  Kind getKind() const { return TheKind; }
  source::Text getName() const { return Name; }

  Scope *getScope() { return TheScope.get(); }
  const Scope *getScope() const { return TheScope.get(); }

  void setScope(std::unique_ptr<Scope> scope) { TheScope = std::move(scope); }

private:
  Kind TheKind;
  source::Text Name;
  std::unique_ptr<Scope> TheScope;
};

llvm::raw_ostream &operator<<(llvm::raw_ostream &, const Symbol &);

/// A semantic Scope containing Symbols
struct Scope final {
  Scope(Scope *parent) : Scope{parent, /*symbol=*/nullptr} {}

  /// Declare a Symbol in this Scope. If a Symbol with the same name exists in
  /// this Scope or any parent Scope, the existing Symbol is returned and the
  /// declaration fails
  std::pair<Symbol *, bool> declare(Symbol::Kind kind, source::Text name) {
    if (Symbol * symbol{lookup(name)}) {
      return {symbol, false};
    }
    Symbol *symbol{&Symbols.try_emplace(name, kind, name).first->second};
    switch (symbol->getKind()) {
    case Symbol::Kind::Function:
      symbol->setScope(std::unique_ptr<Scope>{new Scope{this, symbol}});
      break;
    case Symbol::Kind::Param:
    case Symbol::Kind::Var:
      break;
    }
    return {symbol, true};
  }

  /// Lookup recursively in parent Scopes
  Symbol *lookup(source::Text name) {
    return const_cast<Symbol *>(static_cast<const Scope *>(this)->lookup(name));
  }

  /// Lookup recursively in parent Scopes
  const Symbol *lookup(source::Text name) const {
    for (const Scope *scope{this}; scope; scope = scope->Parent) {
      if (auto it{scope->Symbols.find(name)}; it != scope->Symbols.end()) {
        return &it->second;
      }
    }
    return nullptr;
  }

  /// Return all Symbols declared in this Scope. If a Kind is provided, only
  /// Symbols of that Kind are returned. The result is sorted by source position
  std::vector<const Symbol *>
  getSymbols(std::optional<Symbol::Kind> kind = std::nullopt) const {
    std::vector<const Symbol *> symbols;
    for (const auto &pair : Symbols) {
      if (!kind || pair.second.getKind() == *kind) {
        symbols.push_back(&pair.second);
      }
    }
    llvm::sort(symbols, [](const Symbol *lhs, const Symbol *rhs) {
      return lhs->getName().getRange() < rhs->getName().getRange();
    });
    return symbols;
  }

  Scope *getParent() { return Parent; }
  const Scope *getParent() const { return Parent; }

  Symbol *getSymbol() { return TheSymbol; }
  const Symbol *getSymbol() const { return TheSymbol; }

private:
  Scope(Scope *parent, Symbol *symbol) : Parent{parent}, TheSymbol{symbol} {}

  Scope *Parent;
  Symbol *TheSymbol;
  llvm::StringMap<Symbol, llvm::BumpPtrAllocator> Symbols;
};

llvm::raw_ostream &operator<<(llvm::raw_ostream &, const Scope &);

} // namespace mua::sema

#endif // MUA_SEMA_SYMBOL_H
