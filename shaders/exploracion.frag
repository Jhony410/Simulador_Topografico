#version 330 core
in vec4 vColor;
layout (location = 0) out vec4 FragColor;
layout (location = 1) out vec4 BrilloColor;
void main() {
    FragColor = vColor;
    float energia = max(vColor.r, max(vColor.g, vColor.b)) * 0.35;
    BrilloColor = vec4(vColor.rgb * energia, vColor.a * energia);
}
