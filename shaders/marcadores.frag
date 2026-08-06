#version 330 core

in  float vDistancia;
in  float vAlturaRelativa;
in  vec2  vUVMascara;

layout (location = 0) out vec4 FragColor;
layout (location = 1) out vec4 BrilloColor;

uniform float uAlphaMaximo;
uniform float uRadioNitido;
uniform float uRadioDesvanecido;
uniform float uRadioEscaneo;
uniform sampler2D uMascara;

void main()
{
    // Misma caida que la rejilla pero con radios propios: las torretas deben
    // seguir leyendose a lo lejos, si no el mapa parece vacio.
    float alpha = 1.0 - smoothstep(uRadioNitido, uRadioDesvanecido, vDistancia);
    alpha *= uAlphaMaximo;
    if (alpha < 0.004) discard;

    // ---- Estado ------------------------------------------------------------
    // El estado solo cambia el COLOR, nunca el tamano: la geometria del VBO es
    // estatica y estas torretas no crecen al descubrirse.
    float explorado = 0.0;
    if (vUVMascara.x >= 0.0 && vUVMascara.x <= 1.0 &&
        vUVMascara.y >= 0.0 && vUVMascara.y <= 1.0)
        explorado = smoothstep(0.15, 0.75, texture(uMascara, vUVMascara).r);

    // Bajo el radar del dron destella en ambar; ya explorada queda en cian.
    float bajoRadar = 1.0 - smoothstep(uRadioEscaneo * 0.8, uRadioEscaneo * 1.6, vDistancia);

    // Niebla de guerra: una torreta en zona sin escanear NO existe todavia para
    // el jugador. Si no, quedarian antenas flotando sobre el vacio negro.
    float visible = max(explorado, bajoRadar);
    if (visible < 0.02) discard;
    alpha *= visible;

    vec3 gris = vec3(0.42, 0.48, 0.55);   // no explorada: tenue
    vec3 cian = vec3(0.36, 0.86, 0.92);   // explorada
    vec3 ambar= vec3(1.00, 0.82, 0.24);   // dentro del radar

    vec3 color = mix(gris, cian, explorado);
    color = mix(color, ambar, bajoRadar * 0.75);

    // El mastil se apaga hacia la base para que la torreta no compita con el
    // relieve y se lea apoyada en el suelo, no sobreimpresa.
    float degradado = mix(0.55, 1.0, vAlturaRelativa);
    alpha *= degradado;

    FragColor = vec4(color * alpha, alpha);

    // Solo la luz de la punta (alturaRelativa == 1) aporta al buffer de brillo,
    // y aun asi con poca energia: no deben dominar la escena.
    float esPunta = step(0.995, vAlturaRelativa);
    BrilloColor = vec4(color * esPunta * alpha * 0.55, esPunta * alpha * 0.55);
}
