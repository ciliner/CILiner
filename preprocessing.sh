ROOT=$(pwd)

cd $ROOT


git clone https://github.com/google/AFL.git

cd AFL

rm -r llvm_mode

cp -r $ROOT/impact/llvm_mode/ ./llvm_mode/

make && \
make install && \
make -C llvm_mode

cd $ROOT
