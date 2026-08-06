#version 330 core

in vec4 vColor;

layout (location = 0) out vec4 FragColor;
layout (location = 1) out vec4 BrilloColor;

void main()
{
    if (vColor.a < 0.002) discard;

    // Se dibuja con mezcla ADITIVA (SRC_ALPHA, ONE): el color se premultiplica
    // por su propio alpha, de modo que el haz SUMA luz al fondo en vez de
    // taparlo. Eso es lo que evita que el cono se lea como un solido opaco.
    FragColor = vec4(vColor.rgb * vColor.a, vColor.a);

    // Aporte contenido al buffer de brillo: el halo debe insinuar el haz, no
    // convertirlo en una mancha blanca sobre el terreno.
    BrilloColor = vec4(vColor.rgb * vColor.a * 0.45, vColor.a * 0.45);
}
