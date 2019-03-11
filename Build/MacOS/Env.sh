#!/bin/bash
echo "To Setup build aliases execute 'source Build/MacOS/Env.sh' in terminal from Methane root"

alias init="cmake -H. -B ./Build/Output/XCode -G Xcode -DCMAKE_BUILD_TYPE=Debug"
alias build="cmake --build ./Build/Output/XCode"
alias run="open ./Build/Output/XCode/Apps/Tutorials/03-ShadowCube/Debug/MethaneShadowCube.app"
