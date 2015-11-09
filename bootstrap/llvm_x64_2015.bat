@echo off

PATH=%PATH%;%CD%\Tools\cmake-3.3.2-win32-x86\bin;%CD%\Tools\python27

call "%VS140COMNTOOLS%\..\..\vc\vcvarsall.bat" x86_amd64
mkdir llvm-vs2015_x64
cd llvm-vs2015_x64
cmake -G "Visual Studio 14 Win64" ..\llvm
msbuild llvm.sln /m /t:tools\llvm-objdump /p:Configuration="Debug"
msbuild llvm.sln /m /t:tools\llvm-objdump /p:Configuration="Release"
cd ..
