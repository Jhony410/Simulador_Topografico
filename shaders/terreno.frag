#version 330 core

in vec3 vPosicionMundo;
in vec3 vNormal;
in float vAltura;
in vec2 vUVTerreno;

layout (location = 0) out vec4 FragColor;
layout (location = 1) out vec4 BrilloColor;

uniform vec3 uColorBase;
uniform float uAlphaMaximo;
uniform vec3 uPosicionDron;
uniform vec3 uPosicionCamara;
uniform float uRadioNitido;
uniform float uRadioDesvanecido;
uniform int uModo;
uniform int uPasada;
uniform bool uCurvas;
uniform float uTiempo;
uniform sampler2D uExploracion;

vec3 rampaElevacion(float h) {
    vec3 bajo = vec3(0.04, 0.36, 0.55);
    vec3 medio = vec3(0.10, 0.86, 0.78);
    vec3 alto = vec3(1.00, 0.72, 0.12);
    return h < 0.5 ? mix(bajo, medio, h * 2.0) : mix(medio, alto, (h - 0.5) * 2.0);
}

void main() {
    if (uPasada == 1 && length(gl_PointCoord - vec2(0.5)) > 0.5) discard;

    float distanciaDron = length(vPosicionMundo.xz - uPosicionDron.xz);
    float foco = 1.0 - smoothstep(uRadioNitido, uRadioDesvanecido, distanciaDron);
    float descubierto = texture(uExploracion, clamp(vUVTerreno, 0.0, 1.0)).r;
    float revelado = max(0.16, max(descubierto, foco * 0.82));
    if (uModo == 5) revelado = max(0.06, max(descubierto, foco));

    vec3 color = uColorBase;
    if (uModo == 4 || (uModo == 5 && uPasada == 2)) color = rampaElevacion(vAltura);
    else color = mix(vec3(0.18, 0.34, 0.50), uColorBase, 0.35 + 0.65 * vAltura);

    if (uCurvas && uPasada == 2) {
        float banda = abs(fract(vAltura * 14.0) - 0.5);
        float curva = 1.0 - smoothstep(0.42, 0.49, banda);
        color = mix(color, vec3(0.84, 0.94, 1.0), curva * 0.78);
    }

    if (uPasada == 2) {
        vec3 normal = normalize(vNormal);
        float difuso = max(dot(normal, normalize(vec3(-0.35, 0.85, 0.28))), 0.0);
        color *= 0.28 + 0.72 * difuso;
    }

    float distanciaCamara = length(vPosicionMundo - uPosicionCamara);
    float niebla = 1.0 - smoothstep(105.0, 360.0, distanciaCamara);
    float alphaBase = uPasada == 2 ? (uModo == 4 ? 0.78 : 0.24) : uAlphaMaximo;
    float pulso = uModo == 5 ? 0.92 + 0.08 * sin(uTiempo * 2.0 - distanciaDron * 0.25) : 1.0;
    float alpha = alphaBase * revelado * niebla * pulso;
    if (alpha < 0.008) discard;

    FragColor = vec4(color * (0.45 + 0.55 * revelado), alpha);
    float emision = (uPasada != 2 && distanciaDron < uRadioNitido) ? 0.10 : 0.0;
    BrilloColor = vec4(color * emision, alpha * emision);
}
