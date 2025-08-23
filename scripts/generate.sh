#!/bin/bash

export VERSION="$(git describe --always --dirty)"
export VERSION_MAJOR="0"
export VERSION_MINOR="0"
export VERSION_PATCH="0"
export RASCRIPT_NPP_SYNTAX=$(cat temp/rascript.tmLanguage)

if [[ ${GITHUB_REF_TYPE} == "tag" && ${GITHUB_REF_NAME} != "" ]]; then
    VERSION="${GITHUB_REF_NAME#v}"
    VERSION_MAJOR=$(echo "$VERSION" | cut -d'.' -f1)
    VERSION_MINOR=$(echo "$VERSION" | cut -d'.' -f2)
    VERSION_PATCH=$(echo "$VERSION" | cut -d'.' -f3)
fi

cat templates/RAScript.rc.template | envsubst > src/RAScript.rc
cat templates/Config.h.template | envsubst > src/Config.h