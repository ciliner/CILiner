FROM ubuntu:22.04

RUN apt-get update && apt-get install -y \
    wget \
    software-properties-common \
    build-essential \
    git \
    python3 \
    python3-dev \
    make \
    cmake \
    curl \
    bash \
    dos2unix \
    ninja-build \
    libtool \
    libssl-dev \
    libffi-dev \
    libedit-dev \
    libxml2-dev \
    zlib1g-dev \
    pkg-config \
    autoconf \
    automake \
    autoconf-archive \
    locales \ 
    clang

RUN locale-gen en_US.UTF-8
RUN update-locale update-locale LANG=en_US.UTF-8

WORKDIR /root

ARG GITHUB_TOKEN
RUN git clone https://github.com/ciliner/CILiner.git

WORKDIR /root/CILiner

RUN chmod +x build.sh && dos2unix ./build.sh

RUN chmod +x ./build-llvm.sh && dos2unix ./build-llvm.sh
RUN ./build-llvm.sh

ENV PATH="/root/CILiner/llvm-project/prefix/bin/:${PATH}"

RUN ./build.sh


WORKDIR /root/CILiner/

RUN git clone https://github.com/google/AFL.git

RUN rm -r /root/CILiner/AFL/llvm_mode
RUN cp -r /root/CILiner/impact/llvm_mode /root/CILiner/AFL/llvm_mode

WORKDIR /root/CILiner/AFL

RUN make && \
    make install && \
    make -C llvm_mode

CMD ["bash"]
