#pragma once
#include "PuntoEscaneo.h"

#include <glm/glm.hpp>
#include <vector>

class MapaExploracion;
class Terreno;

class SistemaExploracion {
public:
    void generar(const Terreno& terreno, int cantidad, unsigned int semilla);
    void reiniciar();
    void actualizar(const glm::vec3& posicionDron, float velocidadDron,
                    float dt, MapaExploracion& mapa);

    const std::vector<PuntoEscaneo>& obtenerPuntos() const { return puntos; }
    int obtenerCompletados() const { return completados; }
    int obtenerTotal() const { return static_cast<int>(puntos.size()); }
    float obtenerProgresoPuntos() const;
    int obtenerObjetivoActual() const { return objetivoActual; }
    float distanciaAlObjetivo(const glm::vec3& posicion) const;
    bool estaCompleta() const { return !puntos.empty() && completados == static_cast<int>(puntos.size()); }
    float obtenerTiempoMision() const { return tiempoMision; }
    float obtenerDistanciaRecorrida() const { return distanciaRecorrida; }

private:
    int buscarObjetivoMasCercano(const glm::vec3& posicion) const;

    std::vector<PuntoEscaneo> puntos;
    int objetivoActual = -1;
    int completados = 0;
    float tiempoMision = 0.0f;
    float distanciaRecorrida = 0.0f;
    glm::vec3 posicionAnterior{0.0f};
    bool tienePosicionAnterior = false;
};
