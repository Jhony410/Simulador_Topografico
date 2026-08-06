#version 330 core

// Luz de escaneo. La geometria llega ya en coordenadas de MUNDO (el cono, la
// huella y los anillos se generan cada frame siguiendo el relieve real), asi
// que no hay matriz de modelo: solo vista y proyeccion.
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec4 aColor;   // rgb de la luz + alpha ya resuelto en CPU

uniform mat4 view;
uniform mat4 projection;

out vec4 vColor;

void main() {
    vColor = aColor;
    gl_Position = projection * view * vec4(aPos, 1.0);
}
