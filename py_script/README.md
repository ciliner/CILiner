# python Env preparation

## Install miniconda

```bash
# install miniconda

wget https://repo.anaconda.com/miniconda/Miniconda3-latest-Linux-x86_64.sh

bash Miniconda3-latest-Linux-x86_64.sh
```

## Create a new VENV

```bash
conda create -n ciliner python=3.10.8

conda activate impact

python --version
```

## install requirements

```bash
pip install install clang=17.0.6

pip install psutil
```

## run analyzer after CILiner impact analysis

```bash

cd /root/CILiner

conda activate ciliner

python3 py_script/analyzer.py /root/CILiner/dataset/cJSON
```
