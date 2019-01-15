@echo off

if NOT DEFINED BUILD_NUMBER (
  set MAJOR_VER=3
)

if NOT DEFINED BUILD_NUMBER (
  set MINOR_VER=0
)

if NOT DEFINED BUILD_NUMBER (
  set BUILD_NUMBER=0000
)

if NOT DEFINED OUTPUT_PATH (
  set OUTPUT_PATH=%CD%
)

if NOT DEFINED CMAKE_EXE (
  set CMAKE_EXE="C:\Program Files\CMake\bin\cmake.exe"
)

pushd VideoXpertSdkMedia
call ..\Nuget\nuget.exe install packages.config -OutputDirectory packages -ExcludeVersion

if exist build\ rd /q /s build
mkdir build & pushd build

mkdir VxSdkMedia_Win32 & pushd VxSdkMedia_Win32
%CMAKE_EXE% -G "Visual Studio 14" ..\..\
popd

mkdir VxSdkMedia_x64 & pushd VxSdkMedia_x64
%CMAKE_EXE% -G "Visual Studio 14 Win64" ..\..\
popd

popd

%CMAKE_EXE% --build build\VxSdkMedia_Win32 --config Release
%CMAKE_EXE% --build build\VxSdkMedia_x64 --config Release

popd

call Nuget\nuget.exe pack Nuget\VideoXpertSdk-Media.nuspec -Version %MAJOR_VER%.%MINOR_VER%.%BUILD_NUMBER% -OutputDirectory %OUTPUT_PATH%