export NPP_VERSION := v8.9.8.1
export TINYXML2_VERSION := 11.0.0
export BOOSTREGEX_VERSION := boost-1.92.0
export RASCRIPT_SYNTAX_VERSION := v0.4.2

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
	rm -rf src/regex
	rm -f src/RAScript.rc
	rm -f src/RAScript.xml
	git submodule update --init --recursive
	cd src/notepad-plus-plus && git fetch && git checkout tags/${NPP_VERSION}
	cd src/tinyxml2 && git fetch && git checkout tags/${TINYXML2_VERSION}
	cd src/regex && git fetch && git checkout tags/${BOOSTREGEX_VERSION}

compile-x64: generate
	ARCH=x86_64 ./scripts/build.sh

compile-Win32: generate
	ARCH=i686 ./scripts/build.sh

compile-x64-debug: generate
	ARCH=x86_64 ./scripts/build.sh --debug

compile-Win32-debug: generate
	ARCH=i686 ./scripts/build.sh --debug

NPP_X64 := ~/.wine/drive_c/Program\ Files/Notepad++
NPP_WIN32 := ~/.wine/drive_c/Program\ Files\ \(x86\)/Notepad++
DEBUG_OUTPUT := 2>&1 | grep --line-buffered 'OutputDebugStringW'

install: compile-x64
	rm -rf $(NPP_X64)/plugins/RAScript
	mkdir -p $(NPP_X64)/plugins/RAScript
	cp out/RAScript.dll $(NPP_X64)/plugins/RAScript
	make open

install-Win32: compile-Win32
	rm -rf $(NPP_WIN32)/plugins/RAScript
	mkdir -p $(NPP_WIN32)/plugins/RAScript
	cp out/RAScript.dll $(NPP_WIN32)/plugins/RAScript
	make open-Win32

install-debug: compile-x64-debug
	rm -rf $(NPP_X64)/plugins/RAScript
	mkdir -p $(NPP_X64)/plugins/RAScript
	cp out/RAScript.dll $(NPP_X64)/plugins/RAScript
	make open-debug

install-Win32-debug: compile-Win32-debug
	rm -rf $(NPP_WIN32)/plugins/RAScript
	mkdir -p $(NPP_WIN32)/plugins/RAScript
	cp out/RAScript.dll $(NPP_WIN32)/plugins/RAScript
	make open-Win32-debug

open:
	wine $(NPP_X64)/notepad++.exe

open-Win32:
	wine $(NPP_WIN32)/notepad++.exe

open-debug: # shows a Debug build's debug output in the terminal
	WINEDEBUG=warn+seh wine $(NPP_X64)/notepad++.exe $(DEBUG_OUTPUT)

open-Win32-debug: # shows a Debug build's debug output in the terminal
	WINEDEBUG=warn+seh wine $(NPP_WIN32)/notepad++.exe $(DEBUG_OUTPUT)

debug:
	wine ~/.wine/drive_c/Program\ Files/DebugView/dbgview64.exe
