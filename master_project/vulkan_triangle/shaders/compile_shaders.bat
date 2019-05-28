echo "compile shaders"

mkdir compiled

for %%i in (*.frag, *.vert) do (
	%VULKAN_SDK%\Bin32\glslangValidator.exe -V %%i -o %CD%\compiled\%%i.spv
)