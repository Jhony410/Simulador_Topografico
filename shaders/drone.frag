#version 330 core

out vec4 FragColor;

uniform vec3  uColor = vec3(1.0, 0.1, 0.0);
uniform float uAlpha = 1.0;

void main()
{
    FragColor = vec4(uColor, uAlpha);
}
