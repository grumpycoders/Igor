cd wxWidget/build/msw
call "C:\Program Files (x86)\Microsoft Visual Studio 12.0\VC\vcvarsall.bat" x86_amd64
nmake.exe /f makefile.vc CPU=x64 BUILD=release MONOLITHIC=1 DEBUG_INFO=1
nmake.exe /f makefile.vc CPU=x64 BUILD=debug MONOLITHIC=1 DEBUG_INFO=1
cd ../../..