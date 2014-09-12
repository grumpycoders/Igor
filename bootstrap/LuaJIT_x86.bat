@echo off

cd Balau\LuaJIT\src

call "%VS120COMNTOOLS%\..\..\vc\vcvarsall.bat" x86
call ..\..\..\msvcbuild.bat static
copy lua51.lib ..\..\..\lua51_32r.lib
call ..\..\..\msvcbuild.bat debug static
copy lua51.lib ..\..\..\lua51_32d.lib

cd ..\..\..
