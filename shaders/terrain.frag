#version 330 core

out vec4 FragColor;

void main()
{
    // Color plano blanco azulado semi-transparente (estilo Orano Group)
    // Se usa tanto para los puntos (GL_POINTS) como para la malla (wireframe)
    FragColor = vec4(0.85, 0.88, 0.92, 0.7);
}
