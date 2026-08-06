#version 330 core
in vec2 vUV;
out vec4 FragColor;

uniform sampler2D uAltura;    // heightmap normalizado a 0..1 (R8)
uniform sampler2D uMascara;   // mascara de exploracion (R8)
uniform float uNiveles;       // numero de curvas de nivel

// Rampa topografica: bajo -> cian oscuro, medio -> verde, alto -> amarillo,
// cumbre -> naranja. Solo se aplica donde la mascara ya revelo el terreno.
vec3 rampaTopografica(float h) {
    vec3 bajo   = vec3(0.05, 0.24, 0.38);
    vec3 medio  = vec3(0.12, 0.62, 0.42);
    vec3 alto   = vec3(0.78, 0.78, 0.24);
    vec3 cumbre = vec3(0.92, 0.52, 0.16);
    if (h < 0.34) return mix(bajo,  medio,  smoothstep(0.00, 0.34, h));
    if (h < 0.68) return mix(medio, alto,   smoothstep(0.34, 0.68, h));
    return               mix(alto,  cumbre, smoothstep(0.68, 1.00, h));
}

void main() {
    float h = texture(uAltura, vUV).r;

    // ---- Revelado progresivo ----------------------------------------------
    // La mascara llega como R8 con filtrado LINEAR: el propio hardware ya
    // interpola entre celdas vecinas. El smoothstep convierte ese gradiente en
    // un borde suave de descubrimiento en vez de un escalon de celda.
    float bruta = texture(uMascara, vUV).r;
    float revelado = smoothstep(0.10, 0.62, bruta);

    // Fondo de zona sin explorar: azul casi negro. NO se dibuja ni el relieve
    // ni las curvas ahi, para que el mapa se construya al volarlo.
    vec3 oculto = vec3(0.012, 0.026, 0.052);
    vec3 color = oculto;

    if (revelado > 0.002) {
        vec3 terreno = rampaTopografica(h);

        // Curvas de nivel por bandas sobre la altura normalizada.
        float banda = abs(fract(h * uNiveles) - 0.5);
        float curva = 1.0 - smoothstep(0.40, 0.48, banda);
        terreno = mix(terreno, vec3(0.80, 0.94, 0.98), curva * 0.55);

        color = mix(oculto, terreno, revelado);

        // Halo tenue justo en el frente de descubrimiento: da sensacion de
        // barrido activo sin necesidad de geometria extra.
        float frente = revelado * (1.0 - revelado) * 4.0;
        color += vec3(0.10, 0.16, 0.14) * frente;
    }

    // Cuadricula de referencia, siempre visible pero muy tenue: es el "papel"
    // sobre el que se va dibujando el levantamiento.
    float grilla = max(1.0 - smoothstep(0.0, 0.010, abs(fract(vUV.x * 8.0) - 0.5)),
                       1.0 - smoothstep(0.0, 0.010, abs(fract(vUV.y * 8.0) - 0.5)));
    color += vec3(0.030, 0.055, 0.085) * grilla;

    FragColor = vec4(color, 0.92);
}
