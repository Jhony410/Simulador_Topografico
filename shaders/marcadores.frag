#version 330 core

in  float vDistancia;

layout (location = 0) out vec4 FragColor;
layout (location = 1) out vec4 BrilloColor;

uniform vec3  uColorBase;
uniform float uAlphaMaximo;
uniform float uRadioNitido;
uniform float uRadioDesvanecido;

void main()
{
    // Misma caida que la rejilla pero con radios propios: las estacas deben
    // seguir leyendose a lo lejos, si no el mapa parece vacio.
    float alpha = 1.0 - smoothstep(uRadioNitido, uRadioDesvanecido, vDistancia);
    alpha *= uAlphaMaximo;
    if (alpha < 0.004) discard;

    FragColor = vec4(uColorBase * alpha, alpha);
    BrilloColor = vec4(0.0);   // las estacas no emiten
}
