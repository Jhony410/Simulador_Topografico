#version 330 core

// Posicion del vertice (unico atributo en esta fase)
layout (location = 0) in vec3 aPos;

// Matrices del pipeline MVP
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    // Solo transformamos la posicion; no hay color ni normal por vertice en esta fase
    gl_Position = projection * view * model * vec4(aPos, 1.0);
}
