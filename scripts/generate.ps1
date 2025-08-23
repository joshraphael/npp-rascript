$version = git describe --always --dirty
$major = "0"
$minor = "0"
$patch = "0"
$syntaxUrl = "https://github.com/joshraphael/rascript-syntax/releases/download/$Env:RASCRIPT_SYNTAX_VERSION/rascript.tmLanguage"
Invoke-WebRequest -Uri $syntaxUrl -OutFile "temp\rascript.tmLanguage"
$rascriptNppSyntax = Get-Content -Path "temp\rascript.tmLanguage" -Raw

if(($Env:GITHUB_REF_TYPE -eq "tag") -and ($Env:GITHUB_REF_NAME -ne "") -and ($Env:GITHUB_REF_NAME.StartsWith("v"))) {
    $version = $Env:GITHUB_REF_NAME.Substring(1) # remove the leading v
    $values = $version.Split(".")
    $major = $values[0]
    $minor = $values[1]
    $patch = $values[2]
}

Remove-Item -Path "src\RAScript.rc" -Force -ErrorAction SilentlyContinue
get-content templates/RAScript.rc.template | %{$_ -replace "\$\{VERSION_MAJOR\}", $major} | %{$_ -replace "\$\{VERSION_MINOR\}", $minor} | %{$_ -replace "\$\{VERSION_PATCH\}", $patch} | Out-File src/RAScript.rc
get-content templates/Config.h.template | %{$_ -replace "\$\{RASCRIPT_NPP_SYNTAX\}", $rascriptNppSyntax} | Out-File src/Config.h