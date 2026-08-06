#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform int  uPasada;
uniform float uMinAltura;
uniform float uMaxAltura;
uniform vec4 uLimites;

out vec3 vPosicionMundo;
out vec3 vNormal;
out float vAltura;
out vec2 vUVTerreno;

void main() {
    vec4 mundo = model * vec4(aPos, 1.0);
    vPosicionMundo = mundo.xyz;
    vNormal = mat3(transpose(inverse(model))) * aNormal;
    vAltura = clamp((mundo.y - uMinAltura) / max(0.0001, uMaxAltura - uMinAltura), 0.0, 1.0);
    vUVTerreno = vec2((mundo.x - uLimites.x) / max(0.0001, uLimites.y - uLimites.x),
                      (mundo.z - uLimites.z) / max(0.0001, uLimites.w - uLimites.z));
    gl_PointSize = uPasada == 1 ? 2.2 : 1.0;
    gl_Position = projection * view * mundo;
}
