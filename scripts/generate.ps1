$version = git describe --always --dirty
$major = "0"
$minor = "0"
$patch = "0"

if(($Env:GITHUB_REF_TYPE -eq "tag") -and ($Env:GITHUB_REF_NAME -ne "") -and ($Env:GITHUB_REF_NAME.StartsWith("v"))) {
    $version = $Env:GITHUB_REF_NAME.Substring(1) # remove the leading v
    $values = $version.Split(".")
    $major = $values[0]
    $minor = $values[1]
    $patch = $values[2]
}

get-content templates/RAScript.rc.template | %{$_ -replace "\$\{VERSION_MAJOR\}", $major} | %{$_ -replace "\$\{VERSION_MINOR\}", $minor} | %{$_ -replace "\$\{VERSION_PATCH\}", $patch} | Out-File src/RAScript.rc