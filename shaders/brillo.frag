#version 330 core

// Prepara el buffer de brillo antes de desenfocarlo. Se ejecuta a media
// resolucion: el desenfoque posterior cuesta la cuarta parte y, como el
// resultado va a quedar borroso de todos modos, no se pierde nada visible.
in  vec2 vUV;
out vec4 FragColor;

uniform sampler2D uBrillo;   // attachment 1 de la pasada de escena
uniform float     uUmbral;

void main()
{
    vec3 color = texture(uBrillo, vUV).rgb;

    // Corte SUAVE por luminancia: un corte duro haria que el borde del glow
    // parpadeara al moverse el dron.
    float luminancia = dot(color, vec3(0.2126, 0.7152, 0.0722));
    float factor = smoothstep(uUmbral, uUmbral + 0.25, luminancia);

    FragColor = vec4(color * factor, 1.0);
}
