#version 330 core

out vec4 FragColor;

// Color configurable: terreno (blanco azulado) y dron (amarillo)
uniform vec3  uColor = vec3(0.85, 0.88, 0.92);
uniform float uAlpha = 0.7;

void main()
{
    FragColor = vec4(uColor, uAlpha);
}
