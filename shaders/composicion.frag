#version 330 core
in vec2 vUV;
out vec4 FragColor;

uniform sampler2D uColor;
uniform sampler2D uBrillo;
uniform float uIntensidadGlow;
uniform bool uParticulas;
uniform float uTiempo;
uniform vec3 uFondoBajo;
uniform vec3 uFondoAlto;

float hash21(vec2 p) {
    p = fract(p * vec2(123.34, 456.21));
    p += dot(p, p + 45.32);
    return fract(p.x * p.y);
}

void main() {
    vec3 escena = texture(uColor, vUV).rgb;
    vec3 brillo = texture(uBrillo, vUV).rgb;
    vec3 gradiente = mix(uFondoBajo, uFondoAlto, pow(clamp(vUV.y, 0.0, 1.0), 1.45));
    float contenido = smoothstep(0.018, 0.085, length(escena - uFondoBajo));

    float estrellas = 0.0;
    if (uParticulas) {
        vec2 celda = floor(gl_FragCoord.xy / 8.0);
        float ruido = hash21(celda);
        estrellas = smoothstep(0.992, 0.999, ruido) *
                    (0.45 + 0.55 * sin(uTiempo * 0.7 + ruido * 40.0));
    }
    vec3 fondo = gradiente + estrellas * vec3(0.12, 0.22, 0.32) * (1.0 - contenido);
    vec3 color = mix(fondo, escena, contenido) + brillo * uIntensidadGlow;

    vec2 centro = vUV * 2.0 - 1.0;
    float vigneta = 1.0 - smoothstep(0.48, 1.35, dot(centro, centro));
    color *= 0.68 + 0.32 * vigneta;
    FragColor = vec4(color, 1.0);
}
