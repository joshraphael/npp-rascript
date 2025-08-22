#!/bin/bash

if [ -z "${ARCH}" ]; then echo "empty architecture for build"; exit 1; fi;

if [[ ${ARCH} != @(${ARCH}|i686) ]]; then echo "invalid architecture selected to build: ${chapter}"; exit 1; fi;

# DefaultLexer
${ARCH}-w64-mingw32-g++ -std=c++17 -DUNICODE -DDEBUG \
-Isrc/notepad-plus-plus/scintilla/include \
-Isrc/notepad-plus-plus/lexilla/include \
-c src/notepad-plus-plus/lexilla/lexlib/DefaultLexer.cxx -o out/DefaultLexer.o

# LexAccessor
${ARCH}-w64-mingw32-g++ -std=c++17 -DUNICODE -DDEBUG \
-Isrc/notepad-plus-plus/scintilla/include \
-Isrc/notepad-plus-plus/lexilla/lexlib \
-c src/notepad-plus-plus/lexilla/lexlib/LexAccessor.cxx -o out/LexAccessor.o

# StyleContext
${ARCH}-w64-mingw32-g++ -std=c++17 -DUNICODE -DDEBUG \
-Isrc/notepad-plus-plus/scintilla/include \
-Isrc/notepad-plus-plus/lexilla/lexlib \
-c src/notepad-plus-plus/lexilla/lexlib/StyleContext.cxx -o out/StyleContext.o

# WordList
${ARCH}-w64-mingw32-g++ -std=c++17 -DUNICODE -DDEBUG \
-Isrc/notepad-plus-plus/lexilla/lexlib \
-c src/notepad-plus-plus/lexilla/lexlib/WordList.cxx -o out/WordList.o

# Lexer
${ARCH}-w64-mingw32-g++ -std=c++17 -DUNICODE -DDEBUG \
-Isrc/notepad-plus-plus/scintilla/include \
-Isrc/notepad-plus-plus/lexilla/lexlib \
-Isrc/notepad-plus-plus/lexilla/include \
-Isrc/notepad-plus-plus/PowerEditor/src/MISC/PluginsManager \
-c src/Lexer.cpp -o out/Lexer.o

# Resource File
${ARCH}-w64-mingw32-windres src/RAScript.rc -o out/rc.o

# RAScript
${ARCH}-w64-mingw32-g++ -std=c++17 -DUNICODE -DDEBUG \
-Isrc/notepad-plus-plus/PowerEditor/src \
-Isrc/notepad-plus-plus/PowerEditor/src/MISC/PluginsManager \
-Isrc/notepad-plus-plus/scintilla/include \
-Isrc/notepad-plus-plus/lexilla/lexlib \
-Isrc/notepad-plus-plus/lexilla/include \
-c src/RAScript.cpp -o out/RAScript.o

# Link
${ARCH}-w64-mingw32-g++ -static-libstdc++ -static-libgcc -shared -o out/RAScript.dll \
out/DefaultLexer.o \
out/LexAccessor.o \
out/StyleContext.o \
out/WordList.o \
out/Lexer.o \
out/rc.o \
out/RAScript.o \
-Wl,--out-implib=out/RAScript.dll.a
