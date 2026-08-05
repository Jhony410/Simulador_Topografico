#version 330 core

layout (location = 0) in vec3  aPos;
layout (location = 1) in float aPropId; // 0 = cuerpo, 1..4 = helice

uniform mat4  model;
uniform mat4  view;
uniform mat4  projection;
uniform float uTime;        // tiempo para girar las helices
uniform float uSpin;        // velocidad de giro (rad/seg)
uniform vec3  uPivots[4];   // centro de giro de cada helice (espacio local)

void main()
{
    vec3 p = aPos;
    int pid = int(aPropId + 0.5);

    if (pid >= 1) {
        vec3 pivot = uPivots[pid - 1];
        // helices alternas giran en sentido opuesto (como un cuadricoptero real)
        float dir = (pid == 1 || pid == 3) ? 1.0 : -1.0;
        float ang = uTime * uSpin * dir;
        float c = cos(ang), s = sin(ang);
        p -= pivot;
        p = vec3(c * p.x + s * p.z, p.y, -s * p.x + c * p.z); // giro sobre eje Y
        p += pivot;
    }

    gl_Position = projection * view * model * vec4(p, 1.0);
}
