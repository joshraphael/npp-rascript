SHELL := /bin/bash
NPP_VERSION := v8.8.5

deps:
	sudo apt-get install gcc-mingw-w64-x86-64 g++-mingw-w64-x86-64 gcc-mingw-w64-i686 g++-mingw-w64-i686

clean:
	rm -rf out
	mkdir -p out
	rm -rf src/notepad-plus-plus
	git submodule update --init --recursive
	cd src/notepad-plus-plus && git checkout tags/${NPP_VERSION}

compile-x64: clean
	# Plugin Info
	x86_64-w64-mingw32-windres src/RAScript.rc -o out/rc.o

	# RAScript
	x86_64-w64-mingw32-g++ -std=c++17 -DUNICODE \
	-Isrc/notepad-plus-plus/PowerEditor/src \
	-Isrc/notepad-plus-plus/PowerEditor/src/MISC/PluginsManager \
	-Isrc/notepad-plus-plus/scintilla/include \
	-c src/RAScript.cpp -o out/RAScript.o

	# Link
	x86_64-w64-mingw32-g++ -static-libstdc++ -static-libgcc -shared -o out/RAScript.dll out/rc.o out/RAScript.o -Wl,--out-implib=out/RAScript.dll.a

compile-x64: clean
	ARCH=x86_64 ./scripts/build.sh

compile-Win32: clean
	ARCH=i686 ./scripts/build.sh

install: compile-x64
	cp out/RAScript.dll ~/.wine/drive_c/Program\ Files/Notepad++/plugins/RAScript
	make open

open:
	wine ~/.wine/drive_c/Program\ Files/Notepad++/notepad++.exe