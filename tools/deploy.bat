@echo off

REM 部署脚本
setlocal

REM 设置Qt环境变量
call qt5_trae_vars.bat

REM 构建项目
qmake MultiMerge.pro
nmake

REM 复制必要文件
copy release\MultiMerge.exe .\

REM 清理临时文件
del /Q *.obj
rmdir /S /Q release
rmdir /S /Q debug

echo Deployment completed!
goto end

:error
echo Error during deployment!

:end
endlocal
pause