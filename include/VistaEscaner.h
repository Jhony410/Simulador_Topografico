#pragma once
#include "Camara.h"
#include "EscalaMundo.h"
#include "RecursoGL.h"

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>

class GestorRecursos;
class Terreno;

// ============================================================================
//  VISTA: luz de escaneo proyectada desde la panza del dron.
//
//  Se compone de tres elementos, todos con mezcla ADITIVA y sin escritura en el
//  z-buffer, para que se lean como LUZ y no como geometria solida:
//
//    1) Cono semitransparente  - vertice bajo el dron, base justo sobre el
//                                relieve. Su alpha cae del vertice a la base.
//    2) Huella en el suelo     - abanico de triangulos que SIGUE la altura real
//                                del terreno, con atenuacion radial del centro
//                                al borde. Usa polygon offset para no pelear
//                                con la rejilla en el z-buffer.
//    3) Anillos de radar       - circunferencias que se expanden desde el
//                                centro perdiendo alpha, tambien pegadas al
//                                relieve. Se reinician por periodo.
//
//  La altura del cono se calcula cada frame como (dron.y - alturaEn(dron.xz)),
//  asi que la luz TERMINA exactamente en la superficie sea cual sea el relieve.
//
//  Todos los buffers se crean UNA vez con capacidad maxima y despues solo se
//  reescriben con glBufferSubData: no se recrea ningun recurso por frame.
// ============================================================================
class VistaEscaner {
public:
    void inicializar(GestorRecursos& recursos);
    void liberar();

    void avanzar(float dt) { tiempo += dt; }

    // Devuelve el numero de draw calls emitidas.
    int dibujar(const Camara& camara, const Terreno& terreno, const EscalaMundo& escala,
                const glm::vec3& posicionDron, float aspecto, bool activo);

private:
    struct Vertice {
        glm::vec3 posicion;
        glm::vec4 color;      // rgb + alpha ya resuelto en CPU
    };

    // Capacidad maxima: cono (SEGMENTOS*3) + huella (SEGMENTOS*3) + anillos.
    static constexpr int CAPACIDAD_VERTICES = 4096;

    GLuint programa = 0;
    GLint  locVista = -1, locProyeccion = -1;
    VertexArrayGL vao;
    BufferGL      vbo;

    float tiempo = 0.0f;
    std::vector<Vertice> buffer;   // reutilizado cada frame, nunca reasignado
};
