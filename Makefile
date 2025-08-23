export NPP_VERSION := v8.8.5
export RASCRIPT_SYNTAX_VERSION := v0.0.3

deps:
	sudo apt-get install gcc-mingw-w64-x86-64 g++-mingw-w64-x86-64 gcc-mingw-w64-i686 g++-mingw-w64-i686

generate: clean
	./scripts/generate.sh

generate-win: clean
	pwsh.exe .\scripts\generate.ps1

clean: # works on windows and linux, careful changing this
	rm -rf out
	mkdir -p out
	rm -rf src/notepad-plus-plus
	rm -rf temp/
	mkdir -p temp/
	rm -f src/RAScript.rc
	rm -f src/Config.h
	git submodule update --init --recursive
	cd src/notepad-plus-plus && git checkout tags/${NPP_VERSION}

compile-x64: generate
	ARCH=x86_64 ./scripts/build.sh

compile-Win32: generate
	ARCH=i686 ./scripts/build.sh

install: compile-x64
	cp out/RAScript.dll ~/.wine/drive_c/Program\ Files/Notepad++/plugins/RAScript
	make open

open:
	wine ~/.wine/drive_c/Program\ Files/Notepad++/notepad++.exe

debug:
	wine ~/.wine/drive_c/Program\ Files/DebugView/dbgview64.exe