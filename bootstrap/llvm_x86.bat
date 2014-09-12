@echo off

PATH=%PATH%;%CD%\Tools\CMake-2.8\bin;%CD%\Tools\python27

call "%VS120COMNTOOLS%\..\..\vc\vcvarsall.bat" x86
mkdir llvm-build32
cd llvm-build32
cmake -G "Visual Studio 12" ..\llvm
msbuild llvm.sln /m /t:tools\llvm-objdump /p:Configuration="Debug"
msbuild llvm.sln /m /t:tools\llvm-objdump /p:Configuration="Release"
cd ..
