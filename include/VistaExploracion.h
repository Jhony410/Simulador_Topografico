#pragma once
#include "Ajustes.h"
#include "Camara.h"
#include "EscalaMundo.h"
#include "SistemaExploracion.h"
#include "SistemaMedicion.h"
#include "RecursoGL.h"

#include <glad/glad.h>

class GestorRecursos;
class Terreno;
class MapaExploracion;

class VistaExploracion {
public:
    void inicializar(GestorRecursos& recursos);
    // 'mapa' se usa para no dibujar guias sobre terreno todavia sin explorar:
    // una zona de sondeo en la oscuridad seria un anillo flotando en el vacio.
    int dibujar(const Camara& camara, const Terreno& terreno,
                const SistemaExploracion& exploracion, const MapaExploracion& mapa,
                const SistemaMedicion& medicion, const Ajustes& ajustes,
                const EscalaMundo& escala,
                const glm::vec3& posicionDron, float aspecto, float tiempo);
    void liberar();

private:
    GLuint programa = 0;
    VertexArrayGL vao;
    BufferGL vbo;
    GLint locVista = -1, locProyeccion = -1;
};
