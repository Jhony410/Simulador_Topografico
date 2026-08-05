#version 330 core

// Curvas de nivel proyectadas sobre un plano inclinado (el "holograma").
// Las coordenadas llegan ya normalizadas al cuadrado [-1, 1] del plano.
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec3 aColor;   // rampa por altura, calculada en CPU

uniform mat4 model;        // inclinacion del plano (rotacion en X y en Y)
uniform mat4 view;
uniform mat4 projection;   // perspectiva propia del panel

out vec3 vColor;

void main()
{
    vColor = aColor;
    gl_Position = projection * view * model * vec4(aPos, 0.0, 1.0);
}
