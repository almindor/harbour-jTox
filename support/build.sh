#!/bin/bash

# Exit on errors
set -e

if [ "$#" -ne 1 ]; then
    echo "Illegal number of parameters"
    exit 1
fi

if [ "$1" == "i486" ]; then
    TARGET_COMPILER="x86-linux-gcc"
elif [ "$1" == "armv7hl" ]; then
    TARGET_COMPILER="armv7-linux-gcc"
else
    echo "Invalid target"
    exit 2
fi

SFVER="2.2.1.18"
SODIUMVER="1.0.16"
TOXCOREVER="0.2.7"
THREADS="8"
TARGET="$SFVER-$1"
TOXDIR=`pwd`
FAKEDIR="$TOXDIR/$TARGET"

SODIUMDIR="$TOXDIR/libsodium"
SODIUMSRC="https://github.com/jedisct1/libsodium/archive/$SODIUMVER.tar.gz"

TOXCORESRC="https://github.com/TokTok/c-toxcore/archive/v$TOXCOREVER.tar.gz"
TOXCOREDIR="$TOXDIR/c-toxcore"

VPXSRC="https://github.com/webmproject/libvpx/archive/v1.7.0.tar.gz"
VPXDIR="$TOXDIR/vpx"

OPUSSRC="https://github.com/xiph/opus/archive/v1.2.1.tar.gz"
OPUSDIR="$TOXDIR/opus"

if [ ! -d "$FAKEDIR" ]
then
	echo -en "Creating fake root $TARGET.. \t\t"
	mkdir -p "$FAKEDIR"
	echo "OK"
else
	echo -en "Cleaning fake root $TARGET.. \t\t"
	rm -rf "$FAKEDIR"/*
	echo "OK"
fi

rm -rf "$SODIUMDIR" && mkdir -p "$SODIUMDIR"
rm -rf "$TOXCOREDIR" && mkdir -p "$TOXCOREDIR"
rm -rf "$VPXDIR" && mkdir -p "$VPXDIR"
rm -rf "$OPUSDIR" && mkdir -p "$OPUSDIR"

echo -en "Getting libsodium .. \t\t\t"
curl -s -L "$SODIUMSRC" | tar xz -C "$SODIUMDIR" --strip-components=1 &> /dev/null
echo "OK"

echo -en "Getting libvpx .. \t\t\t"
curl -s -L "$VPXSRC" | tar xz -C "$VPXDIR" --strip-components=1 &> /dev/null
echo "OK"

echo -en "Getting libopus .. \t\t\t"
curl -s -L "$OPUSSRC" | tar xz -C "$OPUSDIR" --strip-components=1 &> /dev/null
echo "OK"

echo -en "Getting toxcore .. \t\t\t"
curl -s -L "$TOXCORESRC" | tar xz -C "$TOXCOREDIR" --strip-components=1 &> /dev/null
echo "OK"

# LIBSODIUM
cd "$SODIUMDIR"
echo -en "Building libsodium.. \t\t\t"
sb2 -t SailfishOS-$TARGET -m sdk-build autoreconf -i &> "$TOXDIR/output.log"
sb2 -t SailfishOS-$TARGET -m sdk-build ./configure --prefix "$FAKEDIR" &> "$TOXDIR/output.log"
sb2 -t SailfishOS-$TARGET -m sdk-build make clean &> "$TOXDIR/output.log"
sb2 -t SailfishOS-$TARGET -m sdk-build make -j $THREADS &> "$TOXDIR/output.log"
echo "OK"
echo -en "Installing libsodium to $TARGET.. \t\t"
sb2 -t SailfishOS-$TARGET -m sdk-build make install &> "$TOXDIR/output.log"
echo "OK"
cd "$TOXDIR"

# VPX
cd "$VPXDIR"
echo -en "Building libvpx.. \t\t\t"
sb2 -t SailfishOS-$TARGET -m sdk-build ./configure --enable-pic --prefix="$FAKEDIR" --target="$TARGET_COMPILER" &> "$TOXDIR/output.log"
sb2 -t SailfishOS-$TARGET -m sdk-build make clean &> "$TOXDIR/output.log"
sb2 -t SailfishOS-$TARGET -m sdk-build make -j $THREADS &> "$TOXDIR/output.log"
echo "OK"
echo -en "Installing libvpx to $TARGET.. \t\t"
sb2 -t SailfishOS-$TARGET -m sdk-build make install &> "$TOXDIR/output.log"
echo "OK"

# OPUS
cd "$OPUSDIR"
echo -en "Building libopus.. \t\t\t"
sb2 -t SailfishOS-$TARGET -m sdk-build ./autogen.sh &> "$TOXDIR/output.log"
sb2 -t SailfishOS-$TARGET -m sdk-build ./configure --with-pic --prefix="$FAKEDIR" &> "$TOXDIR/output.log"
sb2 -t SailfishOS-$TARGET -m sdk-build make clean &> "$TOXDIR/output.log"
sb2 -t SailfishOS-$TARGET -m sdk-build make -j $THREADS &> "$TOXDIR/output.log"
echo "OK"
echo -en "Installing libopus to $TARGET.. \t\t"
sb2 -t SailfishOS-$TARGET -m sdk-build make install &> "$TOXDIR/output.log"
echo "OK"

# TOXCORE
cd "$TOXCOREDIR"
echo -en "Building toxcore.. \t\t\t"
sb2 -t SailfishOS-$TARGET -m sdk-build autoreconf -i &> "$TOXDIR/output.log"
sb2 -t SailfishOS-$TARGET -m sdk-build ./configure --with-pic --prefix="$FAKEDIR" --with-dependency-search="$FAKEDIR" &> "$TOXDIR/output.log"
sb2 -t SailfishOS-$TARGET -m sdk-build make clean &> "$TOXDIR/output.log"
sb2 -t SailfishOS-$TARGET -m sdk-build make -j $THREADS &> "$TOXDIR/output.log"
echo "OK"
echo -en "Installing toxcore to $TARGET.. \t\t"
sb2 -t SailfishOS-$TARGET -m sdk-build make install &> "$TOXDIR/output.log"
echo "OK"

