#version 330 core

// Desenfoque gaussiano SEPARABLE: una pasada horizontal y otra vertical.
// Un kernel 2D de 9x9 costaria 81 muestras por pixel; separado en dos pasadas
// de 9 son 18. El resultado es matematicamente el mismo porque la gaussiana es
// un producto de gaussianas en cada eje.
in  vec2 vUV;
out vec4 FragColor;

uniform sampler2D uTextura;
uniform vec2      uDireccion;   // (1/ancho, 0) en horizontal, (0, 1/alto) en vertical

void main()
{
    // Pesos de una gaussiana normalizada (suman 1 contando los simetricos).
    const float pesos[5] = float[](0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216);

    vec3 resultado = texture(uTextura, vUV).rgb * pesos[0];
    for (int i = 1; i < 5; ++i) {
        resultado += texture(uTextura, vUV + uDireccion * float(i)).rgb * pesos[i];
        resultado += texture(uTextura, vUV - uDireccion * float(i)).rgb * pesos[i];
    }
    FragColor = vec4(resultado, 1.0);
}
