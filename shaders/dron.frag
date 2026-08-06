#version 330 core
in vec3 vPosicionMundo;
in vec3 vNormalMundo;
layout (location = 0) out vec4 FragColor;
layout (location = 1) out vec4 BrilloColor;

uniform vec3 uColor = vec3(1.0, 0.82, 0.02);
uniform float uAlpha = 1.0;
uniform float uEmision = 0.0;
uniform vec3 uPosicionCamara;

void main() {
    vec3 color = uColor;
    if (uEmision < 0.5) {
        vec3 n = normalize(vNormalMundo);
        vec3 l = normalize(vec3(-0.4, 0.85, 0.3));
        vec3 v = normalize(uPosicionCamara - vPosicionMundo);
        vec3 h = normalize(l + v);
        float difuso = max(dot(n,l),0.0);
        float especular = pow(max(dot(n,h),0.0),28.0) * 0.32;
        color *= 0.24 + 0.76 * difuso;
        color += vec3(especular);
    }
    FragColor = vec4(color, uAlpha);
    BrilloColor = vec4(uColor * uEmision, uAlpha * uEmision);
}
