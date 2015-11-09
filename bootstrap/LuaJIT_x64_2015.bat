@echo off

cd Balau\LuaJIT\src

call "%VS140COMNTOOLS%\..\..\vc\vcvarsall.bat" x86_amd64
call ..\..\..\bootstrap\msvcbuild.bat static
copy lua51.lib ..\..\..\lua51_64r.lib
call ..\..\..\bootstrap\msvcbuild.bat debug static
copy lua51.lib ..\..\..\lua51_64d.lib

cd ..\..\..
