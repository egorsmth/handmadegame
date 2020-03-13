@echo off

mkdir ..\..\build
pushd ..\..\build
cl -DSLOW_CODE=1 -DINTERNAL_BUILD=1 -Zi ..\handmadehero\code\win32_handmade.cpp user32.lib gdi32.lib
popd 