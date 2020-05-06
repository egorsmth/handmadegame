@echo off

set CommonCompilerFlags=-MTd -nologo -Gm- -GR- -EHa- -Od -Oi -WX -W4 -W4 -wd4201 -wd4100 -wd4189 -wd4456 -wd4459 -DSLOW_CODE=1 -Zi -DINTERNAL_BUILD=1 -DHANDMADE_WIN32=1
set CommonLinkerFlags= -incremental:no -opt:ref user32.lib gdi32.lib winmm.lib
mkdir ..\..\build
pushd ..\..\build

cl %CommonCompilerFlags% ..\handmadehero\code\handmade.cpp -Fmgame_handmade.map /LD /link -incremental:no -opt:ref /PDB:handmade_%random%.pdb /DLL /EXPORT:GameGetSoundSamples /EXPORT:GameUpdateAndRender
cl %CommonCompilerFlags% ..\handmadehero\code\win32_handmade.cpp -Fmwin32_handmade.map /link %CommonLinkerFlags%
popd 