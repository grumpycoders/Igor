cd wxWidget/build/msw
call "C:\Program Files (x86)\Microsoft Visual Studio 12.0\VC\vcvarsall.bat" x86
nmake.exe /f makefile.vc CPU=x86 BUILD=release MONOLITHIC=1 DEBUG_INFO=1
nmake.exe /f makefile.vc CPU=x86 BUILD=debug MONOLITHIC=1 DEBUG_INFO=1
cd ../../..