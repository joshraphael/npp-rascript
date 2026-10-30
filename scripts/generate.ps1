# Example running this file:
# $env:NPP_VERSION = 'v8.9.6.4'; $env:RASCRIPT_SYNTAX_VERSION = 'v0.4.2'; $env:TINYXML2_VERSION = '11.0.0'; $env:BOOSTREGEX_VERSION = 'boost-1.91.0'; .\scripts\generate.ps1

Remove-Item -Path "out" -Force -ErrorAction SilentlyContinue
New-Item -Name "out" -ItemType Directory
Remove-Item -Path "src\notepad-plus-plus" -Recurse -Force -ErrorAction SilentlyContinue
Remove-Item -Path "src\tinyxml2" -Force -Recurse -ErrorAction SilentlyContinue
Remove-Item -Path "src\regex" -Force -Recurse -ErrorAction SilentlyContinue
Remove-Item -Path "src\RAScript.rc" -Force -ErrorAction SilentlyContinue
Remove-Item -Path "src\RAScript.xml" -Force -ErrorAction SilentlyContinue
git submodule update --init --recursive
cd src/notepad-plus-plus
git checkout tags/$Env:NPP_VERSION
cd ../..
cd src/tinyxml2
git checkout tags/$Env:TINYXML2_VERSION
cd ../..
cd src/regex
git checkout tags/$Env:BOOSTREGEX_VERSION
cd ../..

$version = git describe --always --dirty
$major = "0"
$minor = "0"
$patch = "0"
$syntaxUrl = "https://github.com/joshraphael/rascript-syntax/releases/download/$Env:RASCRIPT_SYNTAX_VERSION/RAScript.xml"
Invoke-WebRequest -Uri $syntaxUrl -OutFile "src\RAScript.xml"

if(($Env:GITHUB_REF_TYPE -eq "tag") -and ($Env:GITHUB_REF_NAME -ne "") -and ($Env:GITHUB_REF_NAME.StartsWith("v"))) {
    $version = $Env:GITHUB_REF_NAME.Substring(1) # remove the leading v
    $values = $version.Split(".")
    $major = $values[0]
    $minor = $values[1]
    $patch = $values[2]
}

get-content templates/RAScript.rc.template | %{$_ -replace "\$\{VERSION_MAJOR\}", $major} | %{$_ -replace "\$\{VERSION_MINOR\}", $minor} | %{$_ -replace "\$\{VERSION_PATCH\}", $patch} | Out-File src/RAScript.rc
