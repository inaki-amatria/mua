# mua

A toy compiled language with Lua-inspired syntax. Emits object code. The
compiler is called `muac`, the sound of a kiss in Spanish.

## Backstory

I started the project to learn MLIR. After a while, I realized that learning
MLIR without a frontend was a bit boring, so I decided to build a toy language
around it. At some point, the language completely took over the project and the
original goal was mostly forgotten.

Lua was the first language I learned, back when I was fourteen writing bots for
an MMO game, so I wanted to pay tribute to it. The name mua started as a play on
"My own lUA", although it can also be read as "MLIR Lua".

I also thought Lua was an interesting language to combine with MLIR because of
its tables. Since tables can hold different kinds and act as arrays, I was
curious to see whether they could be modeled naturally in the IR rather than
hidden behind runtime library calls.

## Language

Every value is a `double`.

```lua
function factorial(n)
  if n <= 1 then
    return 1
  end
  return n * factorial(n - 1)
end
```

Supports functions, variables, `if`/`elseif`/`else`, arithmetic, comparisons,
and logical operators (`and`, `or`, `not`).

## Build

Builds as an external LLVM project. You need a checkout of
[LLVM](https://github.com/llvm/llvm-project).

```sh
cmake \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo \
  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
  -DCMAKE_C_COMPILER=clang \
  -DCMAKE_CXX_COMPILER=clang++ \
  -DLLVM_USE_LINKER=lld \
  -DLLVM_ENABLE_ASSERTIONS=ON \
  -DLLVM_USE_SANITIZER="Address;Undefined" \
  -DLLVM_TARGETS_TO_BUILD=host \
  -DLLVM_EXTERNAL_PROJECTS="mua" \
  -DLLVM_EXTERNAL_MUA_SOURCE_DIR="/path/to/mua" \
  -Bbuild \
  -GNinja \
  -S/path/to/llvm
ninja -C build check-mua
```

## License

MIT (c) 2026-onwards Iñaki Amatria-Barral
