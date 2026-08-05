#version 330 core

in  vec3 vColor;

layout (location = 0) out vec4 FragColor;
layout (location = 1) out vec4 BrilloColor;

uniform float uAlpha;

void main()
{
    FragColor = vec4(vColor, uAlpha);
    BrilloColor = vec4(0.0);   // el panel no participa del glow
}
