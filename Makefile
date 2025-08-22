SHELL := /bin/bash
NPP_VERSION := v8.8.5

deps:
	sudo apt-get install gcc-mingw-w64-x86-64 g++-mingw-w64-x86-64 gcc-mingw-w64-i686 g++-mingw-w64-i686

generate:
	rm -f src/RAScript.rc
	./scripts/generate.sh

generate-win:
	rm -f src\RAScript.rc
	.\scripts\generate.ps1

clean: # works on windows and linux, careful changing this
	rm -rf out
	mkdir -p out
	rm -rf src/notepad-plus-plus
	git submodule update --init --recursive
	cd src/notepad-plus-plus && git checkout tags/${NPP_VERSION}

compile-x64: generate clean
	ARCH=x86_64 ./scripts/build.sh

compile-Win32: generate clean
	ARCH=i686 ./scripts/build.sh

install: compile-x64
	cp out/RAScript.dll ~/.wine/drive_c/Program\ Files/Notepad++/plugins/RAScript
	make open

open:
	wine ~/.wine/drive_c/Program\ Files/Notepad++/notepad++.exe