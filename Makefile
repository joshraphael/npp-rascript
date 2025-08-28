export NPP_VERSION := v8.8.5
export TINYXML2_VERSION := 11.0.0
export RASCRIPT_SYNTAX_VERSION := v0.0.3

deps:
	sudo apt-get install gcc-mingw-w64-x86-64 g++-mingw-w64-x86-64 gcc-mingw-w64-i686 g++-mingw-w64-i686

generate: clean
	./scripts/generate.sh

generate-win:
	pwsh.exe .\scripts\generate.ps1

clean: # works on windows and linux, careful changing this
	rm -rf out
	mkdir -p out
	rm -rf src/notepad-plus-plus
	rm -rf src/tinyxml2
	rm -rf temp/
	mkdir -p temp/
	rm -f src/RAScript.rc
	rm -f src/Config.hpp
	git submodule update --init --recursive
	cd src/notepad-plus-plus && git checkout tags/${NPP_VERSION}
	cd src/tinyxml2 && git checkout tags/${TINYXML2_VERSION}

compile-x64: generate
	ARCH=x86_64 ./scripts/build.sh

compile-Win32: generate
	ARCH=i686 ./scripts/build.sh

install: compile-x64
	rm -rf ~/.wine/drive_c/Program\ Files/Notepad++/plugins/RAScript
	mkdir -p ~/.wine/drive_c/Program\ Files/Notepad++/plugins/RAScript
	cp out/RAScript.dll ~/.wine/drive_c/Program\ Files/Notepad++/plugins/RAScript
	make open

install-Win32: compile-Win32
	rm -rf ~/.wine/drive_c/Program\ Files\ \(x86\)/Notepad++/plugins/RAScript
	mkdir -p ~/.wine/drive_c/Program\ Files\ \(x86\)/Notepad++/plugins/RAScript
	cp out/RAScript.dll ~/.wine/drive_c/Program\ Files\ \(x86\)/Notepad++/plugins/RAScript
	make open-Win32

open:
	wine ~/.wine/drive_c/Program\ Files/Notepad++/notepad++.exe

open-Win32:
	wine ~/.wine/drive_c/Program\ Files\ \(x86\)/Notepad++/notepad++.exe

debug:
	wine ~/.wine/drive_c/Program\ Files/DebugView/dbgview64.exe