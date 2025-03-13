#!/bin/bash -e

ROOT=$(pwd)

LLVM_SRC_DIR="${ROOT}/llvm-project/llvm"
MY_PASS_DIR="${ROOT}/MyPass"
LLVM_PASS_DIR="${LLVM_SRC_DIR}/lib/Transforms/MyPass"

git clone https://github.com/llvm/llvm-project.git
cd $ROOT/llvm-project
git checkout 6009708b4367171ccdbf4b5905cb6a803753fe18

if [ ! -d "build" ]; then
  mkdir build
fi

cd build

cmake -DLLVM_TARGET_ARCH="X86" \
			-DLLVM_TARGETS_TO_BUILD="ARM;X86;AArch64" \
			-DLLVM_EXPERIMENTAL_TARGETS_TO_BUILD=WebAssembly \
			-DCMAKE_BUILD_TYPE=Release \
			-DLLVM_ENABLE_PROJECTS="clang;lldb" \
			-G "Unix Makefiles" \
			../llvm

make -j$(nproc)

if [ ! -d "$ROOT/llvm-project/prefix" ]; then
  mkdir $ROOT/llvm-project/prefix
fi

cmake -DCMAKE_INSTALL_PREFIX=$ROOT/llvm-project/prefix -P cmake_install.cmake