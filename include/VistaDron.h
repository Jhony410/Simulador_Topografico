#pragma once
#include "Camara.h"
#include "Componentes.h"
#include "MallaCruda.h"

#include <glad/glad.h>

class GestorRecursos;

// ============================================================================
//  VISTA: dibuja el dron en dos pasadas (relleno oscuro + aristas), y pasa al
//  shader los pivotes de las helices para que giren en la GPU.
// ============================================================================
class VistaDron {
public:
    void inicializar(GestorRecursos& recursos);
    void liberar();

    // Sube la malla interleaved [x, y, z, nx, ny, nz, idHelice].
    void subirMalla(const MallaCruda& malla, const glm::vec3 pivotes[4]);

    // Devuelve el numero de draw calls emitidas.
    int dibujar(const Camara& camara, const glm::mat4& matrizModelo,
                 const ComponenteMaterial& material, const ComponenteAnimacion& animacion,
                 float aspecto);

    bool tieneMalla() const { return vao != 0; }

private:
    GLuint programa = 0;
    GLint  locModelo = -1, locVista = -1, locProyeccion = -1;
    GLint  locTiempo = -1, locGiro = -1, locPivotes = -1, locColor = -1, locAlpha = -1;
    GLint  locEmision = -1;
    GLint  locPosicionCamara = -1;

    GLuint vao = 0, vbo = 0, eboTriangulos = 0, eboAristas = 0;
    int    numeroIndices = 0;    // triangulos del relleno
    int    numeroAristas = 0;    // indices del armazon
    glm::vec3 pivotesHelices[4]{};
};
