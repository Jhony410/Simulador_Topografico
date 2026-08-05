#pragma once
#include "Camara.h"
#include "MarcadoresSondeo.h"

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>

class GestorRecursos;

// ============================================================================
//  VISTA: estacas topograficas.
//  Todas viven en UN SOLO VBO con los vertices precalculados. El BVH filtra
//  cuales entran en pantalla y con esa lista se rellena un EBO dinamico, de
//  modo que siguen bastando dos draw calls (postes y cabezas) sin dibujar las
//  estacas que quedan fuera del campo de vision.
// ============================================================================
class VistaMarcadores {
public:
    void inicializar(GestorRecursos& recursos);
    void liberar();

    void subirMarcadores(const MarcadoresSondeo& marcadores);

    // Devuelve el numero de draw calls emitidas.
    int dibujar(const Camara& camara, const glm::mat4& matrizModelo,
                const glm::vec3& posicionDron, float aspecto,
                const std::vector<int>& visibles);

private:
    GLuint programa = 0;
    GLint  locModelo = -1, locVista = -1, locProyeccion = -1;
    GLint  locColorBase = -1, locAlphaMaximo = -1, locPosicionDron = -1;
    GLint  locRadioNitido = -1, locRadioDesvanecido = -1;

    GLuint vao = 0, vbo = 0, ebo = 0;
    int    totalMarcadores = 0;
    int    baseCabezas = 0;   // primer vertice de las cabezas dentro del VBO

    std::vector<unsigned int> indicesPostes;   // reutilizados cada frame
    std::vector<unsigned int> indicesCabezas;
};
