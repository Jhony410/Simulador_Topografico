#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <cstdint>

class GestorRecursos;
class Terreno;
class MapaExploracion;
class SistemaExploracion;

class VistaMinimapa {
public:
    void inicializar(GestorRecursos& recursos);
    void subirTerreno(const Terreno& terreno);
    void actualizarExploracion(const MapaExploracion& mapa);
    int dibujar(const Terreno& terreno, const MapaExploracion& mapa,
                const SistemaExploracion& exploracion,
                const glm::vec3& posicionDron, float yawDron,
                int anchoPantalla, int altoPantalla, bool visible);
    void liberar();

private:
    GLuint programaMapa = 0, programaIconos = 0;
    GLuint vaoMapa = 0, vboMapa = 0;
    GLuint vaoIconos = 0, vboIconos = 0;
    GLuint texturaAltura = 0, texturaMascara = 0;
    std::uint64_t revisionMascara = ~std::uint64_t(0);
};
