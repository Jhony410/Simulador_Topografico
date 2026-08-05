#version 330 core

// Marcadores de sondeo: el mismo VBO guarda los postes (GL_LINES) y las
// cabezas (GL_TRIANGLES). Lo que los distingue es aDesplazamiento: vale (0,0)
// en los vertices del poste y lleva la esquina del cuadrito en la cabeza.
layout (location = 0) in vec3 aPos;            // ancla en el mundo
layout (location = 1) in vec2 aDesplazamiento; // esquina del billboard, en unidades de mundo

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform vec3 uPosicionDron;

out float vDistancia;

void main()
{
    vec4 posicionMundo = model * vec4(aPos, 1.0);
    vDistancia = length(posicionMundo.xz - uPosicionDron.xz);

    // Billboard: el desplazamiento se aplica YA en espacio de vista, por eso el
    // cuadrito siempre encara a la camara sin importar como orbite el jugador.
    vec4 posicionVista = view * posicionMundo;
    posicionVista.xy += aDesplazamiento;

    gl_Position = projection * posicionVista;
}
