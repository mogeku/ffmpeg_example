#!/bin/bash

chcp.com 65001

./configure \
    --toolchain=msvc \
    --prefix=./install \
    --enable-static \
    --disable-shared \
    --disable-doc \
    --disable-manpages \
    --disable-htmlpages \
    --disable-podpages \
    --disable-txtpages \
    --enable-debug \
    --extra-ldflags="-static" \
    --enable-gpl \
    --enable-version3
    # --disable-bzlib \
    # --disable-iconv \
    # --disable-lzma \
    # --disable-mediafoundation \
    # --disable-schannel \
    # --disable-zlib
    # --disable-debug \

make clean
make -j 14

make install
