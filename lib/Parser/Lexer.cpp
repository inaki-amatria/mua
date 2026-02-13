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

#include "Lexer.h"

#include "mua/Source/File.h"
#include "llvm/ADT/StringSwitch.h"

using namespace mua;
using namespace mua::parser;

Lexer::Lexer(const source::File &file)
    : File{file}, Start{File.getBuffer().getBufferStart()},
      End{File.getBuffer().getBufferEnd()}, Current{Start},
      Range{makeRange(/*begin=*/0, /*end=*/0)} {}

Token Lexer::lex() {
  while (true) {
    // Skip whitespace
    while (std::isspace(peek())) {
      advance();
    }
    // Skip comments
    if (peek() == '-' && peek(/*lookahead=*/1) == '-') {
      advance();
      advance();
      while (peek() && peek() != '\n') {
        advance();
      }
      continue;
    }
    break;
  }

  source::Offset begin{getOffset()};

  // EOF
  if (Current >= End) {
    Range = makeRange(begin, getOffset());
    return Token::EndOfFile;
  }

  // Identifier / keyword
  if (std::isalpha(peek()) || peek() == '_') {
    do {
      advance();
    } while (std::isalnum(peek()) || peek() == '_');
    Range = makeRange(begin, getOffset());
    return llvm::StringSwitch<Token>(source::Text{Range})
#define TOKEN(...)
#define KEYWORD(name, spelling) .Case(spelling, Token::name)
#define PUNCT(...)
#include "Token.def"
        .Default(Token::Identifier);
  }

  // Number literal
  if (std::isdigit(peek()) || peek() == '.') {
    bool dotSeen{false};
    do {
      if (peek() == '.') {
        dotSeen = true;
      }
      advance();
    } while (std::isdigit(peek()) || (!dotSeen && peek() == '.'));
    Range = makeRange(begin, getOffset());
    return Token::Number;
  }

  // Single-character punctuation
  Token token{Token::Invalid};
  switch (peek()) {
#define TOKEN(...)
#define KEYWORD(...)
#define PUNCT(name, ch)                                                        \
  case ch:                                                                     \
    token = Token::name;                                                       \
    break;
#include "Token.def"
  }
  advance();
  Range = makeRange(begin, getOffset());
  return token;
}

char Lexer::peek(unsigned lookahead) const {
  const char *p{Current + lookahead};
  if (p >= End) {
    return '\0';
  }
  return *p;
}

void Lexer::advance() {
  if (Current >= End) {
    return;
  }
  ++Current;
}

source::Offset Lexer::getOffset() const {
  return static_cast<source::Offset>(Current - Start);
}

source::Range Lexer::makeRange(source::Offset begin, source::Offset end) const {
  return {File.makePosition(begin), File.makePosition(end)};
}
