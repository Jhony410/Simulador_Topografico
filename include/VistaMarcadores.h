#pragma once
#include "Camara.h"
#include "EscalaMundo.h"
#include "Limites.h"
#include "MarcadoresSondeo.h"

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <cstdint>
#include <vector>

class GestorRecursos;
class MapaExploracion;

// ============================================================================
//  VISTA: torretas topograficas.
//
//  Cada marcador se dibuja como una antena delgada: mastil, cuatro tirantes de
//  base, dos crucetas y una luz minima en la punta. Todas viven en UN SOLO VBO
//  con los vertices precalculados; el BVH filtra cuales entran en pantalla y
//  con esa lista se rellena un EBO dinamico, de modo que siguen bastando dos
//  draw calls sin dibujar las torretas fuera del campo de vision.
//
//  El estado (no explorado / cercano / explorado) NO cambia el tamano fisico:
//  se resuelve en el fragment shader muestreando la mascara de exploracion, asi
//  que no hay que resubir el VBO cuando el jugador descubre terreno.
// ============================================================================
class VistaMarcadores {
public:
    void inicializar(GestorRecursos& recursos);
    void liberar();

    void subirMarcadores(const MarcadoresSondeo& marcadores, const LimitesMundo& limites);
    void actualizarExploracion(const MapaExploracion& mapa);

    // Devuelve el numero de draw calls emitidas.
    int dibujar(const Camara& camara, const glm::mat4& matrizModelo,
                const glm::vec3& posicionDron, float aspecto,
                const EscalaMundo& escala, const std::vector<int>& visibles);

private:
    // Vertices por torreta en cada tramo del VBO (ver subirMarcadores).
    static constexpr int VERTICES_ARMAZON = 18;
    static constexpr int VERTICES_LUZ     = 6;

    GLuint programa = 0;
    GLint  locModelo = -1, locVista = -1, locProyeccion = -1;
    GLint  locAlphaMaximo = -1, locPosicionDron = -1;
    GLint  locRadioNitido = -1, locRadioDesvanecido = -1;
    GLint  locRadioEscaneo = -1, locLimites = -1, locMascara = -1;

    GLuint vao = 0, vbo = 0, ebo = 0;
    GLuint texturaMascara = 0;
    // (minX, maxX, minZ, maxZ): lo necesita el shader para pasar de mundo a UV
    // de la mascara de exploracion.
    glm::vec4 limitesMundo{-50.0f, 50.0f, -50.0f, 50.0f};
    int    totalMarcadores = 0;
    int    baseLuces = 0;   // primer vertice de las luces dentro del VBO
    std::uint64_t revisionMascara = ~std::uint64_t(0);

    std::vector<unsigned int> indicesArmazon;   // reutilizados cada frame
    std::vector<unsigned int> indicesLuces;
};
