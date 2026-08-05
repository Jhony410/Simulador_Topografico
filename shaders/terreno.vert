#version 330 core

// Rejilla del terreno: solo posicion. El relieve ya viene horneado en Y.
layout (location = 0) in vec3 aPos;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

// La atenuacion se calcula por fragmento, asi que hay que llevar la posicion
// de mundo hasta el fragment shader.
out vec3 vPosicionMundo;

void main()
{
    vec4 posicionMundo = model * vec4(aPos, 1.0);
    vPosicionMundo = posicionMundo.xyz;
    gl_Position = projection * view * posicionMundo;
}
