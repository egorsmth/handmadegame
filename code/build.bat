@echo off

set CommonCompilerFlags=-MT -nologo -Gm- -GR- -EHa- -Od -Oi -WX -W4 -W4 -wd4201 -wd4100 -wd4189 -wd4456 -wd4459 -DSLOW_CODE=1 -DINTERNAL_BUILD=1 -Zi
set CommonLinkerFlags=user32.lib gdi32.lib winmm.lib
mkdir ..\..\build
pushd ..\..\build

cl %CommonCompilerFlags% ..\handmadehero\code\win32_handmade.cpp /link %CommonLinkerFlags%
popd 