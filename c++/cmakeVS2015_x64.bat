:: カレントパスをSOURCE_DIRにセット
set SOURCE_DIR=%~dp0

:: buildディレクトリをSOURCE_DIRにセット
set BUILD_DIR=%SOURCE_DIR%\build

:: ビルド構成（コンパイラ、アーキテクチャ）を指定
set GENERATOR_NAME="Visual Studio 14 2015 Win64"

:: パスの指定（PATH = %OpenCV_PATH%\bin)
:setx OpenCV_PATH %%OpenCV_DIR%%\x86\vc14
:setx OpenCV_PATH %%OpenCV_DIR%%\x64\vc14

mkdir %BUILD_DIR%
cd %BUILD_DIR%

:: CMakeによるプロジェクト生成
cmake.exe -G %GENERATOR_NAME% "%BUILD_DIR%" "%SOURCE_DIR%"
::cmake.exe -G %GENERATOR_NAME% --build "%BUILD_DIR%" "%SOURCE_DIR%"

::キー入力によりビルド　d:debug r:release b:両方ビルド
@echo off
echo Build project
echo e:exit
echo r:release build
echo d:debug build
echo other:both build
set /P INPUT=">"

if %INPUT%==e (
echo "exit" 
pause 
exit
) else if %INPUT%==r (
echo "release" 
cmake --build . --config Release 
pause 
exit
) else if %INPUT%==d (
echo "debug" 
cmake --build . --config Debug
pause 
exit
) else (
echo "both" 
cmake --build . --config Debug
cmake --build . --config Release
pause
exit
)