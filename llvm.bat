@echo off

PATH=%PATH%;%CD%\Tools\CMake-2.8\bin;%CD%\python27

call "%VS120COMNTOOLS%\..\..\vc\vcvarsall.bat" x86
mkdir llvm-build32
cd llvm-build32
cmake -G "Visual Studio 12" ..\llvm
msbuild llvm.sln /m /t:tools\llvm-objdump /p:Configuration="Debug"
msbuild llvm.sln /m /t:tools\llvm-objdump /p:Configuration="Release"
cd ..

call "%VS120COMNTOOLS%\..\..\vc\vcvarsall.bat" x86_amd64
mkdir llvm-build64
cd llvm-build64
cmake -G "Visual Studio 12 Win64" ..\llvm
msbuild llvm.sln /m /t:tools\llvm-objdump /p:Configuration="Debug"
msbuild llvm.sln /m /t:tools\llvm-objdump /p:Configuration="Release"
cd ..
