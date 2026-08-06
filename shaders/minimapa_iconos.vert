#version 330 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec4 aColor;
layout (location = 2) in float aTamano;
uniform mat4 uProj;
out vec4 vColor;
void main() { vColor = aColor; gl_PointSize = aTamano; gl_Position = uProj * vec4(aPos,0,1); }
