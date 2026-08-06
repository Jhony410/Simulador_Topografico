#version 330 core

// Torretas de sondeo: el mismo VBO guarda el armazon (GL_LINES) y las luces de
// punta (GL_TRIANGLES). Lo que los distingue es aDesplazamiento: vale (0,0) en
// los vertices del armazon y lleva la esquina del cuadrito en la luz.
layout (location = 0) in vec3  aPos;            // ancla en el mundo
layout (location = 1) in vec2  aDesplazamiento; // esquina del billboard, en unidades de mundo
layout (location = 2) in float aAlturaRelativa; // 0 en la base, 1 en la punta

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform vec3 uPosicionDron;
uniform vec4 uLimites;   // (minX, maxX, minZ, maxZ) del terreno

out float vDistancia;
out float vAlturaRelativa;
out vec2  vUVMascara;

void main()
{
    vec4 posicionMundo = model * vec4(aPos, 1.0);
    vDistancia = length(posicionMundo.xz - uPosicionDron.xz);
    vAlturaRelativa = aAlturaRelativa;

    // Mundo -> UV de la mascara de exploracion, para saber si esta torreta cae
    // en una zona ya descubierta.
    vUVMascara = vec2((posicionMundo.x - uLimites.x) / max(1e-4, uLimites.y - uLimites.x),
                      (posicionMundo.z - uLimites.z) / max(1e-4, uLimites.w - uLimites.z));

    // Billboard: el desplazamiento se aplica YA en espacio de vista, por eso el
    // cuadrito siempre encara a la camara sin importar como orbite el jugador.
    vec4 posicionVista = view * posicionMundo;
    posicionVista.xy += aDesplazamiento;

    gl_Position = projection * posicionVista;
}
