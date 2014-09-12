cd wxWidgets/build/msw
call "%VS120COMNTOOLS%\..\..\vc\vcvarsall.bat" x86
nmake.exe /f makefile.vc CPU=x86 BUILD=release MONOLITHIC=1 DEBUG_INFO=1
nmake.exe /f makefile.vc CPU=x86 BUILD=debug MONOLITHIC=1 DEBUG_INFO=1
cd ../../..