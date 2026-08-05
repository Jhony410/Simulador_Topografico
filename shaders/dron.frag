#version 330 core

// Salida multiple (MRT): todo lo que se dibuja en la pasada de escena escribe
// en DOS attachments a la vez. El 0 es la imagen normal; el 1 recoge solo lo
// que debe brillar, y es el que luego se desenfoca.
layout (location = 0) out vec4 FragColor;
layout (location = 1) out vec4 BrilloColor;

uniform vec3  uColor   = vec3(1.0, 0.898, 0.0);
uniform float uAlpha   = 1.0;
uniform float uEmision = 0.0;   // 0 en la pasada de relleno, 1 en la de aristas

void main()
{
    FragColor = vec4(uColor, uAlpha);

    // El alfa tambien se multiplica por la emision: con uEmision = 0 el alfa
    // queda en 0 y el blending deja intacto lo que ya hubiera en el buffer de
    // brillo, en vez de machacarlo con negro.
    BrilloColor = vec4(uColor * uEmision, uAlpha * uEmision);
}
