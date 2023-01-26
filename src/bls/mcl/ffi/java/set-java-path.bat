@echo off
if "%JAVA_HOME%"=="" (
	set JAVA_DIR=c:/p/Java/jdk
) else (
	set JAVA_DIR=%JAVA_HOME%
)
echo JAVA_DIR=%JAVA_DIR%
rem set PATH=%PATH%;%JAVA_DIR%\bin
