%cd%\shaders\glslangValidator.exe -o defaultVert.spv -V %cd%\shaders\VertexShader.vert
%cd%\shaders\glslangValidator.exe -o defaultFrag.spv -V %cd%\shaders\FragmentShader.frag
move %cd%\*.spv %cd%\shaders\compiled\
exit 0