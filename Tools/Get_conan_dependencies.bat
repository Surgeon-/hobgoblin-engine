@ECHO OFF 

SET SFML_COMMITHASH=8ec114832c0069125f51a87bd5c5adf4632742e5
SET SFML_VERSION=2.5.1

SET LIBZT_COMMITHASH=34992581e3bf33a0ed15ca89bdc1d5bcbbd2dfd4
SET LIBZT_VERSION=1.1.0

SET ZTCPP_COMMITHASH=546ca818f28c0e9988b05fd77684226e7d1ab9cd
SET ZTCPP_VERSION=1.2.0

ECHO "Looking for git..."
where git
IF %ERRORLEVEL% EQU 0 GOTO foundgit
EXIT /b 1
:foundgit

ECHO "Looking for conan..."
where conan
IF %ERRORLEVEL% EQU 0 GOTO foundconan
EXIT /b 1
:foundconan

ECHO "All required tools found!"

MKDIR Build.Temp
CD Build.Temp

ECHO "Getting SFML..."
git clone https://github.com/bincrafters/conan-sfml.git
CD conan-sfml
git checkout %SFML_COMMITHASH%
conan export . %SFML_VERSION%@jbatnozic/stable
CD ..

ECHO "Getting libzt..."
git clone https://github.com/jbatnozic/libzt-conan
CD libzt-conan
git checkout %LIBZT_COMMITHASH%
conan export . %LIBZT_VERSION%@jbatnozic/stable
CD ..

ECHO "Getting ZTCpp..."
git clone https://github.com/jbatnozic/ztcpp
CD ztcpp
git checkout %ZTCPP_COMMITHASH%
conan export . %ZTCPP_VERSION%@jbatnozic/stable
CD ..

ECHO "All required Conan recipes exported!"

REM Clean up
CD ..
DEL /f /s /q Build.Temp 1>NUL
RMDIR /s /q Build.Temp