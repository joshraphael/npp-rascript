#!/bin/bash

if [ -z "${ARCH}" ]; then echo "empty architecture for build"; exit 1; fi;

if [[ ${ARCH} != @(x86_64|i686) ]]; then echo "invalid architecture selected to build: ${chapter}"; exit 1; fi;

${ARCH}-w64-mingw32-windres src/RAScript.rc -o out/rc.o

# RAScript
${ARCH}-w64-mingw32-g++ -std=c++17 -DUNICODE \
-Isrc/notepad-plus-plus/PowerEditor/src \
-Isrc/notepad-plus-plus/PowerEditor/src/MISC/PluginsManager \
-Isrc/notepad-plus-plus/scintilla/include \
-c src/RAScript.cpp -o out/RAScript.o

# Link
${ARCH}-w64-mingw32-g++ -static-libstdc++ -static-libgcc -shared -o out/RAScript.dll out/rc.o out/RAScript.o -Wl,--out-implib=out/RAScript.dll.a