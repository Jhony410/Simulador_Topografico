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
// (inicio, fin) de la niebla de distancia, en unidades de mundo. Se derivan de
// la diagonal del terreno para que la profundidad se lea igual en todo mapa.
uniform vec2 uNiebla;
// Radio del radar. Es lo unico que el dron ilumina por si mismo: fuera de el,
// el relieve solo se ve si YA fue escaneado.
uniform float uRadioRevelado;

vec3 rampaElevacion(float h) {
    vec3 bajo = vec3(0.04, 0.36, 0.55);
    vec3 medio = vec3(0.10, 0.86, 0.78);
    vec3 alto = vec3(1.00, 0.72, 0.12);
    return h < 0.5 ? mix(bajo, medio, h * 2.0) : mix(medio, alto, (h - 0.5) * 2.0);
}

void main() {
    if (uPasada == 1 && length(gl_PointCoord - vec2(0.5)) > 0.5) discard;

    float distanciaDron = length(vPosicionMundo.xz - uPosicionDron.xz);

    // ---- Niebla de guerra: el mapa arranca a OSCURAS -----------------------
    // Sin suelo minimo. Lo que no se ha escaneado no se dibuja en absoluto: el
    // relieve va apareciendo a medida que el radar lo cubre, y una vez visto
    // permanece visible porque la mascara es acumulativa.
    float descubierto = texture(uExploracion, clamp(vUVTerreno, 0.0, 1.0)).r;
    // Halo propio del radar, ligeramente mayor que la mascara: es el frente de
    // descubrimiento que va por delante del dron.
    float halo = 1.0 - smoothstep(uRadioRevelado * 0.80, uRadioRevelado * 1.25, distanciaDron);
    float revelado = max(descubierto, halo);

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
    float niebla = 1.0 - smoothstep(uNiebla.x, uNiebla.y, distanciaCamara);
    float alphaBase = uPasada == 2 ? (uModo == 4 ? 0.78 : 0.24) : uAlphaMaximo;
    float pulso = uModo == 5 ? 0.92 + 0.08 * sin(uTiempo * 2.0 - distanciaDron * 0.25) : 1.0;
    // Atenuacion radial: lo ya explorado pero lejano se lee mas apagado que lo
    // que se tiene debajo. Es estetica, no niebla de guerra.
    float atenuacion = mix(0.42, 1.0, 1.0 - smoothstep(uRadioNitido, uRadioDesvanecido, distanciaDron));
    float alpha = alphaBase * revelado * atenuacion * niebla * pulso;
    if (alpha < 0.008) discard;

    FragColor = vec4(color * (0.45 + 0.55 * revelado), alpha);
    float emision = (uPasada != 2 && distanciaDron < uRadioNitido) ? 0.10 : 0.0;
    BrilloColor = vec4(color * emision, alpha * emision);
}
