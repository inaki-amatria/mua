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

#include "mua/Source/File.h"

#include "llvm/Support/raw_ostream.h"

using namespace mua;
using namespace mua::source;

File::File(std::unique_ptr<llvm::MemoryBuffer> buffer)
    : Buffer{std::move(buffer)} {
  const char *begin{Buffer->getBufferStart()};
  const char *end{Buffer->getBufferEnd()};

  LineOffsets.push_back(0);
  for (const char *p{begin}; p < end; ++p) {
    if (*p == '\n') {
      LineOffsets.push_back(p - begin + 1);
    }
  }
}

std::unique_ptr<File> File::Open(llvm::StringRef filename,
                                 llvm::raw_ostream &os) {
  llvm::ErrorOr<std::unique_ptr<llvm::MemoryBuffer>> buffer{
      llvm::MemoryBuffer::getFile(filename, /*IsText=*/true,
                                  /*RequiresNullTerminator=*/false)};
  if (std::error_code ec{buffer.getError()}) {
    os << "error: could not open file " << filename << ": " << ec.message()
       << '\n';
    return nullptr;
  }
  return std::unique_ptr<File>{new File{std::move(*buffer)}};
}

llvm::MemoryBufferRef File::getBuffer() const { return *Buffer; }

llvm::StringRef File::getFilename() const {
  return Buffer->getBufferIdentifier();
}

llvm::StringRef File::slice(Range range) const {
  assert(range.getFile() == this);
  return Buffer->getBuffer().slice(range.getBegin().getOffset(),
                                   range.getEnd().getOffset());
}

llvm::StringRef File::getLineAt(Position position) const {
  assert(position.getFile() == this);

  unsigned line{getLineAndColumn(position).first};
  Offset begin{LineOffsets[line]};
  Offset end{(line + 1 < LineOffsets.size())
                 ? LineOffsets[line + 1]
                 : static_cast<Offset>(Buffer->getBufferSize())};

  return Buffer->getBuffer().substr(begin, end - begin).rtrim();
}

std::pair<unsigned, unsigned> File::getLineAndColumn(Position position) const {
  assert(position.getFile() == this);

  Offset offset{position.getOffset()};

  // Find the last line whose offset <= position
  const auto it{llvm::upper_bound(LineOffsets, offset)};
  assert(it != LineOffsets.begin());

  unsigned line{static_cast<unsigned>(it - LineOffsets.begin() - 1)};
  unsigned column{static_cast<unsigned>(offset - LineOffsets[line])};

  return {line, column};
}

void File::print(Range range, llvm::raw_ostream &os) const {
  assert(range.getFile() == this);

  Position begin{range.getBegin()};
  Position end{range.getEnd()};

  unsigned column{getLineAndColumn(begin).second};
  llvm::StringRef line{getLineAt(begin)};

  os << range << '\n' << line << '\n';

  unsigned rawLen{static_cast<unsigned>(end.getOffset() - begin.getOffset())};
  unsigned maxLen{static_cast<unsigned>(line.size() - column)};
  unsigned len{rawLen ? std::min(rawLen, maxLen) : 1};

  os.indent(column) << std::string(len ? len : 1, '^');
}

Position File::makePosition(Offset offset) const {
  assert(offset <= Buffer->getBufferSize());
  return {offset, *this};
}
