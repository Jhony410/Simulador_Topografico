#version 330 core
in vec2 vUV;
out vec4 FragColor;
uniform sampler2D uAltura;
uniform sampler2D uMascara;
void main() {
    float h = texture(uAltura, vUV).r;
    float explorado = texture(uMascara, vUV).r;
    vec3 bajo = vec3(0.025, 0.13, 0.22);
    vec3 alto = vec3(0.18, 0.68, 0.74);
    vec3 color = mix(bajo, alto, h);
    float banda = abs(fract(h * 12.0) - 0.5);
    float curva = 1.0 - smoothstep(0.40, 0.48, banda);
    color = mix(color, vec3(0.68, 0.88, 0.94), curva * 0.62);
    color *= mix(0.20, 1.0, explorado);
    float grilla = max(1.0 - smoothstep(0.0, 0.012, abs(fract(vUV.x * 8.0) - 0.5)),
                       1.0 - smoothstep(0.0, 0.012, abs(fract(vUV.y * 8.0) - 0.5)));
    color += vec3(0.05, 0.11, 0.16) * grilla;
    FragColor = vec4(color, 0.91);
}
