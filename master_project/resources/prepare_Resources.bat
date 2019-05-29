echo "compile shader and copy to %CD%"
call %~dp0shaders/compile_shaders.bat %1

echo "copy images from shader %~dp0images to %CD%"
for %%i in (%~dp0images\*) do copy /Y  %%i %CD%\%%~nxi


