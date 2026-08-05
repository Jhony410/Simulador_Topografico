#version 330 core

// Composicion final: la escena tal cual mas el halo desenfocado, sumado.
// La suma (y no una mezcla alfa) es lo que hace que el glow parezca luz
// emitida y no una capa de pintura translucida encima.
in  vec2 vUV;
out vec4 FragColor;

uniform sampler2D uColor;           // attachment 0: escena sin tocar
uniform sampler2D uBrillo;          // halo ya desenfocado
uniform float     uIntensidadGlow;

void main()
{
    vec3 color  = texture(uColor,  vUV).rgb;
    vec3 brillo = texture(uBrillo, vUV).rgb;
    FragColor = vec4(color + brillo * uIntensidadGlow, 1.0);
}
