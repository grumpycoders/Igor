@echo off

cd Balau\LuaJIT\src

call "%VS120COMNTOOLS%\..\..\vc\vcvarsall.bat" x86
call msvcbuild.bat static
copy lua51.lib ..\..\..\lua51_32.lib

call "%VS120COMNTOOLS%\..\..\vc\vcvarsall.bat" x86_amd64
call msvcbuild.bat static
copy lua51.lib ..\..\..\lua51_64.lib

cd ..\..\..
