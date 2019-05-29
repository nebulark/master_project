Rem first argument is the location for th shaders

echo "compile shaders"

rem mkdir compiled

for %%i in (%~dp0*.frag, %~dp0*.vert) do %VULKAN_SDK%\Bin32\glslangValidator.exe -V %%i -o %1%%~nxi.spv