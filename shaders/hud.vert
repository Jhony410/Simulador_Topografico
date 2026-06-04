#version 330 core

layout (location = 0) in vec2 aPos; // coordenadas en pixeles

uniform mat4 uProj;   // ortografica (0,w,h,0)
uniform mat4 uModel;  // traslacion/escala del texto o rectangulo

void main()
{
    gl_Position = uProj * uModel * vec4(aPos, 0.0, 1.0);
}
