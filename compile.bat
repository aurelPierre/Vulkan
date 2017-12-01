%cd%\..\Data\shaders\glslangValidator.exe -o defaultVert.spv -V %cd%\..\Data\shaders\VertexShader.vert
%cd%\..\Data\shaders\glslangValidator.exe -o defaultFrag.spv -V %cd%\..\Data\shaders\FragmentShader.frag
move %cd%\*.spv %cd%\..\Data\shaders\compiled\
exit 0