#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <cstdint>

class GestorRecursos;
class Terreno;
class MapaExploracion;

// ============================================================================
//  VISTA: minimapa topografico con revelado progresivo.
//
//  NO es una imagen fija: el relieve sale del heightmap real subido como
//  textura R8 y lo que decide que se ve es la mascara de exploracion, tambien
//  R8, que crece mientras el dron sobrevuela terreno nuevo. Al arrancar la
//  mascara esta a cero y el minimapa se ve practicamente vacio.
//
//  La textura de la mascara se ASIGNA una sola vez por mapa (glTexImage2D) y a
//  partir de ahi solo se reescribe su contenido (glTexSubImage2D), y ademas
//  solo cuando el Modelo cambia de revision: no se recrea ningun recurso de
//  OpenGL por frame.
// ============================================================================
class VistaMinimapa {
public:
    void inicializar(GestorRecursos& recursos);
    void subirTerreno(const Terreno& terreno);
    void actualizarExploracion(const MapaExploracion& mapa);

    // Devuelve el rectangulo (x, y, ancho, alto) que ocupa en pantalla, para
    // que el HUD alinee debajo el porcentaje y la barra de progreso.
    static glm::vec4 rectangulo(float anchoPantalla, float altoPantalla);

    int dibujar(const Terreno& terreno, const MapaExploracion& mapa,
                const glm::vec3& posicionDron, float yawDron,
                int anchoPantalla, int altoPantalla, bool visible);
    void liberar();

private:
    GLuint programaMapa = 0, programaIconos = 0;
    GLuint vaoMapa = 0, vboMapa = 0;
    GLuint vaoIconos = 0, vboIconos = 0;
    GLuint texturaAltura = 0, texturaMascara = 0;
    GLint  locNiveles = -1;
    int    anchoMascara = 0, altoMascara = 0;
    std::uint64_t revisionMascara = ~std::uint64_t(0);
};
