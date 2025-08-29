#!/bin/bash

if [ -z "${ARCH}" ]; then echo "empty architecture for build"; exit 1; fi;

if [[ ${ARCH} != @(x86_64|i686) ]]; then echo "invalid architecture selected to build: ${ARCH}"; exit 1; fi;

ENABLE_DEBUG=true
DBUG=""
if ${ENABLE_DEBUG}; then
    DBUG=-DDEBUG
fi;
BUILD_OPTIONS="-std=c++17 -O3 -Wall -Werror -Wextra -DUNICODE"

# TinyXML2
${ARCH}-w64-mingw32-g++ -std=c++17 ${BUILD_OPTIONS} ${DBUG} \
-Isrc/tinyxml2 \
-c src/tinyxml2/tinyxml2.cpp -o out/tinyxml2.o

# DefaultLexer
${ARCH}-w64-mingw32-g++ -std=c++17 ${BUILD_OPTIONS} ${DBUG} \
-Isrc/notepad-plus-plus/scintilla/include \
-Isrc/notepad-plus-plus/lexilla/include \
-c src/notepad-plus-plus/lexilla/lexlib/DefaultLexer.cxx -o out/DefaultLexer.o

# LexAccessor
${ARCH}-w64-mingw32-g++ -std=c++17 ${BUILD_OPTIONS} ${DBUG} \
-Isrc/notepad-plus-plus/scintilla/include \
-Isrc/notepad-plus-plus/lexilla/lexlib \
-c src/notepad-plus-plus/lexilla/lexlib/LexAccessor.cxx -o out/LexAccessor.o

# StyleContext
${ARCH}-w64-mingw32-g++ -std=c++17 ${BUILD_OPTIONS} ${DBUG} \
-Isrc/notepad-plus-plus/scintilla/include \
-Isrc/notepad-plus-plus/lexilla/lexlib \
-c src/notepad-plus-plus/lexilla/lexlib/StyleContext.cxx -o out/StyleContext.o

# WordList
${ARCH}-w64-mingw32-g++ -std=c++17 ${BUILD_OPTIONS} ${DBUG} \
-Isrc/notepad-plus-plus/lexilla/lexlib \
-c src/notepad-plus-plus/lexilla/lexlib/WordList.cxx -o out/WordList.o

# Parser (Requires BOOST_REGEX_STANDALONE definition to compile boost regex to improve performance)
${ARCH}-w64-mingw32-g++ -std=c++17 ${BUILD_OPTIONS} ${DBUG} \
-DBOOST_REGEX_STANDALONE \
-Isrc/tinyxml2 \
-Isrc/regex/include \
-Isrc/notepad-plus-plus/scintilla/include \
-Isrc/notepad-plus-plus/PowerEditor/src/MISC/PluginsManager \
-c src/Parser.cpp -o out/Parser.o

# LexRAScript
${ARCH}-w64-mingw32-g++ -std=c++17 ${BUILD_OPTIONS} ${DBUG} \
-Isrc/tinyxml2 \
-Isrc/notepad-plus-plus/scintilla/include \
-Isrc/notepad-plus-plus/lexilla/lexlib \
-Isrc/notepad-plus-plus/lexilla/include \
-Isrc/notepad-plus-plus/PowerEditor/src/MISC/PluginsManager \
-c src/LexRAScript.cpp -o out/LexRAScript.o

# Resource File
${ARCH}-w64-mingw32-windres src/RAScript.rc -o out/rc.o

# RAScript
${ARCH}-w64-mingw32-g++ -std=c++17 ${BUILD_OPTIONS} ${DBUG} \
-Isrc/notepad-plus-plus/PowerEditor/src \
-Isrc/notepad-plus-plus/PowerEditor/src/MISC/PluginsManager \
-Isrc/notepad-plus-plus/scintilla/include \
-Isrc/notepad-plus-plus/lexilla/lexlib \
-Isrc/notepad-plus-plus/lexilla/include \
-c src/RAScript.cpp -o out/RAScript.o

# Link
${ARCH}-w64-mingw32-g++ -static-libstdc++ -static-libgcc -shared -o out/RAScript.dll \
out/tinyxml2.o \
out/DefaultLexer.o \
out/LexAccessor.o \
out/StyleContext.o \
out/WordList.o \
out/Parser.o \
out/LexRAScript.o \
out/rc.o \
out/RAScript.o \
src/Exports.def \
-Wl,--enable-stdcall-fixup \
-Wl,--out-implib=out/RAScript.dll.a
