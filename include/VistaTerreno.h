#pragma once
#include "Camara.h"
#include "Componentes.h"
#include "Terreno.h"

#include <glad/glad.h>
#include <vector>

class GestorRecursos;

// ============================================================================
//  VISTA: dibuja el terreno como una rejilla de lineas con atenuacion radial.
//  Los indices de TODOS los nodos del quadtree viven en un unico EBO estatico;
//  cada nodo visible se dibuja como un tramo (offset, cuenta) de ese buffer, sin
//  reconstruir nada por frame.
// ============================================================================
class VistaTerreno {
public:
    void inicializar(GestorRecursos& recursos);
    void liberar();

    // Sube a la GPU la rejilla y el EBO del quadtree (o las calles, en CSV).
    void subirMalla(const Terreno& terreno);

    // 'nodosVisibles' son indices dentro del quadtree, ya filtrados por frustum
    // y nivel de detalle. Devuelve el numero de draw calls emitidas.
    int dibujar(const Camara& camara, const glm::mat4& matrizModelo,
                const ComponenteMaterial& material, bool topologiaLineas,
                const glm::vec3& posicionDron, float aspecto,
                const Quadtree& quadtree, const std::vector<int>& nodosVisibles);

private:
    GLuint programa = 0;
    GLint  locModelo = -1, locVista = -1, locProyeccion = -1;
    GLint  locColorBase = -1, locAlphaMaximo = -1;
    GLint  locPosicionDron = -1, locRadioNitido = -1, locRadioDesvanecido = -1;

    GLuint vaoRejilla = 0, vboRejilla = 0, eboRejilla = 0;
    GLuint vaoCalles  = 0, vboCalles  = 0, eboCalles  = 0;
    int    indicesCalles = 0;
};
