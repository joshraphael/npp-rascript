# npp-rascript

Notepad++ plugin to define RAScript grammar

[![GitHub License](https://img.shields.io/github/license/joshraphael/npp-rascript)](https://github.com/joshraphael/npp-rascript/blob/main/LICENSE)
[![pipeline](https://github.com/joshraphael/npp-rascript/actions/workflows/release.yaml/badge.svg)](https://github.com/joshraphael/npp-rascript/actions/workflows/release.yaml)
[![pipeline](https://github.com/joshraphael/npp-rascript/actions/workflows/test.yaml/badge.svg)](https://github.com/joshraphael/npp-rascript/actions/workflows/test.yaml)
[![GitHub Tag](https://img.shields.io/github/v/tag/joshraphael/npp-rascript)](https://github.com/joshraphael/npp-rascript/tags)
[![GitHub repo size](https://img.shields.io/github/repo-size/joshraphael/npp-rascript)](https://github.com/joshraphael/npp-rascript/archive/main.zip)

## Setup

This plugin pulls the raw source code from [notepad-plus-plus](https://github.com/notepad-plus-plus/notepad-plus-plus) using git submodules. You cna use the pre defined make commands to build the DLL of the plugin and move it to your plugins folder. A provided [RAScript.vcxproj](RAScript.vcxproj) file is also included for those that use Windows to build this module.

**Note:** If building on Windows, be sure to generate the [RAScript.rc file located in the templates folder](templates/RAScript.rc.template) over to the `src/` folder before trying to build

### LSP
This language uses [rascript-language-server](https://github.com/joshraphael/rascript-language-server) to handle language hints and code tools, download the latest release and place the file somewhere you can remember on your computer (ex: `C:\Users\joshraphael\rascript-language-server_v0.0.1_win-x64.exe`)

install the [NppLspClient](https://github.com/Ekopalypse/NppLspClient) plugin for and place it in your plagins folder. When starting up Notepad++ after installation go to `Plugins > NppLspClient > Open configuration file` and paste in the following configuration for RAScript

```toml
[lspservers."rascript"]
mode = "io"
executable = 'C:\Users\joshraphael\rascript-language-server_v0.0.1_win-x64.exe'
auto_start_server = true
```

Save and restart your editor and your RAScripts should start getting code tools.

## Make Commands

| Title                 | Command              | Parameters | Description                                            |
|-----------------------|----------------------|------------|--------------------------------------------------------|
| Linux Dependencies    | `make deps`          | None       | Download the Linux binaries to build a Windows C++ DLL |
| Compile 64-bit Plugin | `make compile-x64`   | None       | Builds the x64 version of the plugin                   |
| Compile 32-bit Plugin | `make compile-Win32` | None       | Builds the Win32 version of the plugin                 |
| Install               | `make install`       | None       | Install the extension locally (under wine defaults)    |