#version 330 core

in  vec3 vPosicionMundo;

// MRT: attachment 0 imagen normal, attachment 1 buffer de brillo.
layout (location = 0) out vec4 FragColor;
layout (location = 1) out vec4 BrilloColor;

uniform vec3  uColorBase;          // blanco azulado de la rejilla
uniform float uAlphaMaximo;        // opacidad en la zona nitida

uniform vec3  uPosicionDron;       // centro de la zona nitida
uniform float uRadioNitido;        // hasta aqui se ve al 100%
uniform float uRadioDesvanecido;   // aqui ya se fundio con el fondo

void main()
{
    // Distancia HORIZONTAL: si se midiera en 3D, las crestas altas se
    // apagarian antes que el valle que tienen justo al lado.
    float distancia = length(vPosicionMundo.xz - uPosicionDron.xz);

    float alpha = 1.0 - smoothstep(uRadioNitido, uRadioDesvanecido, distancia);
    alpha *= uAlphaMaximo;

    // Recortar lo casi invisible ahorra muchisimo blending sobre el fondo.
    if (alpha < 0.004) discard;

    // El color tambien se multiplica por alpha: la caida hacia el negro queda
    // mas marcada que con transparencia sola, que es el look buscado.
    FragColor = vec4(uColorBase * alpha, alpha);

    // El terreno no emite: alfa 0 para que el blending conserve el brillo que
    // ya hubiera escrito el dron.
    BrilloColor = vec4(0.0);
}
