# Industrial projects

## Dataset

| No | Project | Github Repo |
| :---: | :---: | :---: |
| 1 | [libxml2] | [libxml2](https://github.com/GNOME/libxml2) |
| 2 | [openssl] | [openssl](https://github.com/openssl/openssl.git) |
| 3 | [zstd] | [zstd](https://github.com/facebook/zstd.git) |
| 4 | [usrsctp] | [usrsctp](https://github.com/sctplab/usrsctp.git) |
| 5 | [libhtp] | [libhtp](https://github.com/OISF/libhtp.git) |
| 6 | [libcap] | [libcap](https://github.com/mhiramat/libcap.git) |
| 7 | [cJSON] | [cJSON](https://github.com/DaveGamble/cJSON.git) |
| 8 | [sqlite] | [sqlite](https://github.com/sqlite/sqlite.git) |
| 9 | [sqlcipher] | [sqlcipher](https://github.com/sqlcipher/sqlcipher.git) |
| 10 | [kilo] | [kilo](https://github.com/antirez/kilo.git) |

### build kilo

```shell
git clone https://github.com/antirez/kilo.git

cd kilo

make CC=/root/CILiner/AFL/afl-clang-fast CFLAGS="-g -O0"
```

## build cJSON

```shell
git clone https://github.com/DaveGamble/cJSON.git

cd cJSON

mkdir build

cd build

cmake -DCMAKE_C_COMPILER=/root/CILiner/AFL/afl-clang-fast \
      -DCMAKE_CXX_COMPILER=/root/CILiner/AFL/afl-clang-fast++ \
      -DCMAKE_C_FLAGS="-g -O0 -Wall -Wextra" \
      -DCMAKE_CXX_FLAGS="-g -O0 -Wall -Wextra" \
      ..

make
```

## build libxml2

```shell
cd dataset

git clone https://github.com/GNOME/libxml2

cd libxml2

./autogen.sh

CC=/root/CILiner/AFL/afl-clang-fast CFLAGS="-g -O0" CXXFLAGS="-g -O0" ./configure

make

python3 /root/CILiner/run.py /root/CILiner/build/impact_analysis /root/CILiner/test/bcoutput buf.c:545 -n
```

## build openssl

```shell
git clone https://github.com/openssl/openssl.git

cd openssl

CC=/root/CILiner/AFL/afl-clang-fast CXX=/root/CILiner/AFL/afl-clang-fast++ ./Configure --prefix=/usr/local/ssl --openssldir=/usr/local/ssl '-Wl,-rpath,$(LIBRPATH)'

make

python3 /root/CILiner/run.py /root/CILiner/build/impact_analysis /root/CILiner/test/bcoutput e_dasync.c:734 -n
```

## build zstd

```shell
git clone https://github.com/facebook/zstd.git

cd zstd

make CC=/root/CILiner/AFL/afl-clang-fast CXX=/root/IA/AFL/afl-clang-fast++

python3 /root/CILiner/run.py /root/CILiner/build/impact_analysis /root/CILiner/test/bcoutput zdict.c:689 -n
```

## build usrsctp

```shell
git clone https://github.com/sctplab/usrsctp.git

cd usrsctp

./bootstrap

mkdir build

cd build

make clean

cmake -DCMAKE_C_COMPILER=/root/CILiner/AFL/afl-clang-fast \
      -DCMAKE_CXX_COMPILER=/root/CILiner/AFL/afl-clang-fast++ \
      -DCMAKE_C_FLAGS="-g -O0 -Wall -Wextra" \
      -DCMAKE_CXX_FLAGS="-g -O0 -Wall -Wextra" \
      ..

make

python3 /root/CILiner/run.py /root/CILiner/build/impact_analysis /root/CILiner/test/bcoutput client.c:83 -n
```

## build libhtp

```shell
git clone https://github.com/OISF/libhtp.git

cd libhtp

sudo chmod u+x autogen.sh

./autogen.sh

CC=/root/CILiner/AFL/afl-clang-fast CXX=/root/CILiner/AFL/afl-clang-fast++ CFLAGS="-g -O0" CXXFLAGS="-g -O0" ./configure

make

python3 /root/CILiner/run.py /root/CILiner/build/impact_analysis /root/CILiner/test/bcoutput htp_hooks.c:135 -n
```

### build sqlcipher

```shell
git clone https://github.com/sqlcipher/sqlcipher.git

cd sqlcipher

./configure CC=/root/CILiner/AFL/afl-clang-fast CFLAGS="-g -O0" CXX=/root/CILiner/AFL/afl-clang-fast++ CXXFLAGS="-g -O0"

make
```

## build libcap

```shell
git clone https://github.com/mhiramat/libcap.git

cd libcap

make clean

make CC=/root/IA/AFL/afl-clang-fast CFLAGS="-g -O0"
```

## build sqlite

```shell
git clone https://github.com/sqlite/sqlite.git

cd sqlite

./configure CC=/root/CILiner/AFL/afl-clang-fast CFLAGS="-g -O0" CXX=/root/CILiner/AFL/afl-clang-fast++ CXXFLAGS="-g -O0"

make
```

### how to generate frama-c command

```shell
# first generate selected SuT bc code file using CiLiner's pre-processing module

python3 py_script/frama-c/frama_command.py -p <SuT root directory> -b <bcoutput directory>
```

### how to compile the SuT for dg and sympas

#### dg

```shell
# first generate my_compile_commands.json file using bear



bear --output my_compile_commands.json -- make

python3 py_script/DG/autocompile.py -c <path to my_compile_commands.json> -o <path to bitcode output directory>

python3 py_script/DG/combine.py <path to bitcode output directory>
```

#### sympas

```shell

bear -o my_compile_commands.json -- make


python3 py_script/sympas/autocompile.py -c <path to my_compile_commands.json> -o <path to bitcode output directory>

python3 py_script/sympas/combine.py <path to bitcode output directory>
```
