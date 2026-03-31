@echo off

REM Qt 5.9 环境变量设置
set QT_PATH=C:\Qt\5.9\msvc2015_64
set PATH=%QT_PATH%\bin;%PATH%
set QMAKESPEC=win32-msvc

echo Qt 5.9 environment variables set!