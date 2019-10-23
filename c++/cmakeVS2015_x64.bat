:: �J�����g�p�X��SOURCE_DIR�ɃZ�b�g
set SOURCE_DIR=%~dp0

:: build�f�B���N�g����SOURCE_DIR�ɃZ�b�g
set BUILD_DIR=%SOURCE_DIR%\build

:: �r���h�\���i�R���p�C���A�A�[�L�e�N�`���j���w��
set GENERATOR_NAME="Visual Studio 14 2015 Win64"

:: �p�X�̎w��iPATH = %OpenCV_PATH%\bin)
:setx OpenCV_PATH %%OpenCV_DIR%%\x86\vc14
:setx OpenCV_PATH %%OpenCV_DIR%%\x64\vc14

mkdir %BUILD_DIR%
cd %BUILD_DIR%

:: CMake�ɂ��v���W�F�N�g����
cmake.exe -G %GENERATOR_NAME% "%BUILD_DIR%" "%SOURCE_DIR%"
::cmake.exe -G %GENERATOR_NAME% --build "%BUILD_DIR%" "%SOURCE_DIR%"

::�L�[���͂ɂ��r���h�@d:debug r:release b:�����r���h
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