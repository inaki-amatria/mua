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

#ifndef MUA_SOURCE_POSITION_H
#define MUA_SOURCE_POSITION_H

#include <cassert>
#include <cstdint>

namespace llvm {
class StringRef;
class Twine;
class raw_ostream;
} // namespace llvm

namespace mua::source {

// Forward declaration of File so Position and Range can hold pointers to it
class File;

/// Type alias for byte Offsets into a File's buffer. Used as the canonical
/// coordinate for Position
using Offset = std::uint32_t;

/// Represents a Position in a File
class Position final {
  Position(Offset offset, const File &file)
      : TheOffset{offset}, TheFile{&file} {}
  friend class File; // Only File can create Positions

public:
  Offset getOffset() const { return TheOffset; }
  const File *getFile() const { return TheFile; }

  bool operator<(const Position &other) const {
    return TheFile == other.TheFile && TheOffset < other.TheOffset;
  }

private:
  Offset TheOffset;
  const File *TheFile;
};

/// Represents a half-open range [begin, end) in a File
struct Range final {
  Range(Position begin, Position end) : Begin{begin}, End{end} {
    assert(Begin.getFile() == End.getFile());
    assert(Begin.getOffset() <= End.getOffset());
  }

  Position getBegin() const { return Begin; }
  Position getEnd() const { return End; }
  const File *getFile() const { return Begin.getFile(); }

  bool operator<(const Range &other) const { return Begin < other.Begin; }

private:
  Position Begin;
  Position End;
};

llvm::raw_ostream &operator<<(llvm::raw_ostream &, Range);

/// Represents a contiguous slice of source code in a File
struct Text final {
  Text(Range range) : TheRange{range} {}

  Range getRange() const { return TheRange; }
  const File *getFile() const { return TheRange.getFile(); }

  operator llvm::StringRef() const;
  operator llvm::Twine() const;

private:
  Range TheRange;
};

llvm::raw_ostream &operator<<(llvm::raw_ostream &, Text);

llvm::Twine operator+(const char *, Text);
llvm::Twine operator+(Text, const char *);

} // namespace mua::source

#endif // MUA_SOURCE_POSITION_H
