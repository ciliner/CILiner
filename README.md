# CILiner user guide

## Build

### build docker file

```bash
docker build -t ciliner:latest .

docker run -it ciliner:latest

# building docker from scratch may take a while due to the compilation of the whole LLVM project, so we recommend using the pre-built docker image
```

### Pre-requisites of CILiner (Python Environment)

```bash
wget https://repo.anaconda.com/miniconda/Miniconda3-latest-Linux-x86_64.sh

chmod +x Miniconda3-latest-Linux-x86_64.sh

./Miniconda3-latest-Linux-x86_64.sh

conda create -n ciliner python=3.10.8

conda activate ciliner

python --version

pip install libclang==17.0.6

pip install psutil
```

## Download docker image

```bash

docker pull ciliner/ciliner:latest
```

## RUN

### Pre-precessing

```bash
export CC=/root/CILiner/AFL/afl-clang-fast
export CXX=/root/CILiner/AFL/afl-clang-fast++

```

Example usage for preprocessing of cJSON project:

```bash
cd cd /root/CILiner

# make sure these two directories exist to save the bitcode file for analysis and ll file for reference
mkdir -p test/bcoutput
mkdir -p test/lloutput

cd /root/CILiner/dataset

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

i.e. replace the c compiler and c++ compiler with the AFL compiler, and add the debug flags to the original build command.

```bash
#c_compiler_path
/root/CILiner/AFL/afl-clang-fast

#c++_compiler_path
/root/CILiner/AFL/afl-clang-fast++
```

### RUN main analysis module

```bash
./build/impact_analysis <bcoutput dir> <file:inst_no> [-n]


bcoutput dir: the directory where the bitcode files are stored

file:inst_no: the file and instruction number to be analyzed

-n: (Optional) to disable the output of the intermediate analysis information to reduce the IO overhead of the analysis, if not specified, all the intermediate analysis information will be saved in the ./result/tempinfo directory
```

Example usage for main analysis of cJSON project:

```bash
cd /root/CILiner

python3 ./py_script/run.py ./build/impact_analysis /root/CILiner/test/bcoutput cJSON.c:2364 -n

# the result showing on default output will show the execution time of the analysis including IO overhead and the maximum memory usage during the analysis
```

### RUN source line tracker

```bash
cd /root/CILiner

conda activate ciliner

python3 py_script/analyzer.py <path to the source code>

```

Example usage for source line tracker to cJSON project:

```bash
cd /root/CILiner

conda activate ciliner

python3 py_script/analyzer.py /root/CILiner/dataset/cJSON
```

### Final output

```
The final output of the extracted impact set will be saved in "/root/CILiner/result/final_result.txt"
```

Note: Sometimes, the final result may include a line 0 entry due to the additional debug information inserted in the bitcode file or because some instructions lack associated source line information under the selected optimization level during the compilation process. Simply ignore any occurrences of line 0 in the final result.
