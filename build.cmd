@echo off

if NOT DEFINED MAJOR_VER (
  set MAJOR_VER=3
)

if NOT DEFINED MINOR_VER (
  set MINOR_VER=0
)

if NOT DEFINED BUILD_NUMBER (
  set BUILD_NUMBER=0000
)

if NOT DEFINED OUTPUT_PATH (
  set OUTPUT_PATH="%CD%\output"
)

if NOT DEFINED CMAKE_EXE (
  set CMAKE_EXE="C:\Program Files\CMake\bin\cmake.exe"
)

REM Unzip libgstlibav.dll if it doesn't exist
set GST_LIB_PATH="%CD%\VideoXpertSdkMedia\ThirdParty\GStreamer\1.14.4\x64\gstreamer_runtime\lib\gstreamer-1.0"
if not exist "%GST_LIB_PATH%\libgstlibav.dll" (
  powershell -nologo -noprofile -command "& { Add-Type -A 'System.IO.Compression.FileSystem'; [IO.Compression.ZipFile]::ExtractToDirectory('%GST_LIB_PATH%\libgstlibav.zip', '%GST_LIB_PATH%'); }"
)

REM Create the output path for the NuGet package if it doesn't exist
if not exist %OUTPUT_PATH% mkdir %OUTPUT_PATH%

REM Install NuGet dependencies
pushd VideoXpertSdkMedia
call ..\Nuget\nuget.exe install packages.config -OutputDirectory packages -ExcludeVersion

REM Setup VideoXpertSdkMedia build
if exist build\ rd /q /s build
mkdir build & pushd build

mkdir VxSdkMedia_Win32 & pushd VxSdkMedia_Win32
%CMAKE_EXE% -G "Visual Studio 14" ..\..\
popd

mkdir VxSdkMedia_x64 & pushd VxSdkMedia_x64
%CMAKE_EXE% -G "Visual Studio 14 Win64" ..\..\
popd
popd

REM Build VideoXpertSdkMedia
%CMAKE_EXE% --build build\VxSdkMedia_Win32 --config Release
%CMAKE_EXE% --build build\VxSdkMedia_x64 --config Release
popd

REM Create Nuget Package
call Nuget\nuget.exe pack Nuget\VideoXpertSdk-Media.nuspec -Version %MAJOR_VER%.%MINOR_VER%.%BUILD_NUMBER% -OutputDirectory %OUTPUT_PATH%