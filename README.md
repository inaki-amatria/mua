# mua

## Building mua

```
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
```
