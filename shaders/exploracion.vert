#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec4 aColor;
layout (location = 2) in float aTamano;
uniform mat4 view;
uniform mat4 projection;
out vec4 vColor;
void main() {
    vColor = aColor;
    gl_PointSize = aTamano;
    gl_Position = projection * view * vec4(aPos, 1.0);
}
