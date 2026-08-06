#pragma once
#include "Ajustes.h"
#include "Camara.h"
#include "SistemaExploracion.h"
#include "SistemaMedicion.h"
#include "RecursoGL.h"

#include <glad/glad.h>

class GestorRecursos;
class Terreno;

class VistaExploracion {
public:
    void inicializar(GestorRecursos& recursos);
    int dibujar(const Camara& camara, const Terreno& terreno,
                const SistemaExploracion& exploracion,
                const SistemaMedicion& medicion, const Ajustes& ajustes,
                const glm::vec3& posicionDron, float aspecto, float tiempo);
    void liberar();

private:
    GLuint programa = 0;
    VertexArrayGL vao;
    BufferGL vbo;
    GLint locVista = -1, locProyeccion = -1;
};
