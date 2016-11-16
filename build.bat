@echo off


IF "%1"=="opt" (
	set OptimizationFlags=/Ox
) ELSE (
	set OptimizationFlags=/Od /Oi
)

set CommonCompilerFlags=/MD /nologo /fp:fast /Gm- /GR- /EHa- /WX /W4 /wd4201 /FC /Z7 /D_CRT_SECURE_NO_WARNINGS /Iimgui /Iimgui\examples\libs\gl3w /Iimgui\examples\libs\glfw\include
set CommonLinkerFlags=/incremental:no /opt:ref /LIBPATH:imgui\examples\libs\glfw\lib-vc2010-64 glfw3.lib user32.lib gdi32.lib winmm.lib opengl32.lib shell32.lib kernel32.lib

cl.exe %CommonCompilerFlags% %OptimizationFlags% source/win32_main.cpp /Fec8emu.exe /link %CommonLinkerFlags%
