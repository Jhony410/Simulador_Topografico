#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in float aPropId;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform float uTime;
uniform float uSpin;
uniform vec3 uPivots[4];

out vec3 vPosicionMundo;
out vec3 vNormalMundo;

void main() {
    vec3 p = aPos;
    vec3 n = aNormal;
    int pid = int(aPropId + 0.5);
    if (pid >= 1) {
        vec3 pivot = uPivots[pid - 1];
        float dir = (pid == 1 || pid == 3) ? 1.0 : -1.0;
        float ang = uTime * uSpin * dir;
        float c = cos(ang), s = sin(ang);
        p -= pivot;
        p = vec3(c*p.x+s*p.z, p.y, -s*p.x+c*p.z);
        p += pivot;
        n = vec3(c*n.x+s*n.z, n.y, -s*n.x+c*n.z);
    }
    vec4 mundo = model * vec4(p, 1.0);
    vPosicionMundo = mundo.xyz;
    vNormalMundo = normalize(mat3(transpose(inverse(model))) * n);
    gl_Position = projection * view * mundo;
}
