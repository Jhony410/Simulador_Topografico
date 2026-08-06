#include "SistemaExploracion.h"
#include "MapaExploracion.h"
#include "Terreno.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>
#include <random>

namespace {
constexpr float RADIO_CERCANO = 11.0f;
constexpr float DURACION_ESCANEO = 3.0f;
constexpr float VELOCIDAD_MAX_ESCANEO = 14.0f;
constexpr float DISTANCIA_MINIMA_RELATIVA = 0.16f;
}

void SistemaExploracion::generar(const Terreno& terreno, int cantidad, unsigned int semilla) {
    puntos.clear();
    completados = 0;
    objetivoActual = -1;
    tiempoMision = 0.0f;
    distanciaRecorrida = 0.0f;
    tienePosicionAnterior = false;
    if (!terreno.estaCargado() || cantidad <= 0) return;

    const LimitesMundo& lim = terreno.obtenerLimites();
    std::mt19937 rng(semilla);
    std::uniform_real_distribution<float> ux(lim.minX + lim.ancho() * 0.08f,
                                              lim.maxX - lim.ancho() * 0.08f);
    std::uniform_real_distribution<float> uz(lim.minZ + lim.profundidad() * 0.08f,
                                              lim.maxZ - lim.profundidad() * 0.08f);
    const float separacion = std::min(lim.ancho(), lim.profundidad()) * DISTANCIA_MINIMA_RELATIVA;
    const float pasoPendiente = std::max(0.5f, std::min(lim.ancho(), lim.profundidad()) / 180.0f);

    for (int intento = 0; intento < cantidad * 120 && static_cast<int>(puntos.size()) < cantidad; ++intento) {
        float x = ux(rng), z = uz(rng);
        float h = terreno.alturaEn(x, z);
        float hx = terreno.alturaEn(x + pasoPendiente, z) - terreno.alturaEn(x - pasoPendiente, z);
        float hz = terreno.alturaEn(x, z + pasoPendiente) - terreno.alturaEn(x, z - pasoPendiente);
        float pendiente = std::sqrt(hx * hx + hz * hz) / (2.0f * pasoPendiente);
        if (pendiente > 1.15f) continue;

        bool demasiadoCerca = false;
        for (const auto& p : puntos) {
            if (glm::length(glm::vec2(x - p.posicion.x, z - p.posicion.z)) < separacion) {
                demasiadoCerca = true;
                break;
            }
        }
        if (demasiadoCerca) continue;

        PuntoEscaneo punto;
        punto.id = static_cast<int>(puntos.size()) + 1;
        punto.posicion = glm::vec3(x, h, z);
        punto.radio = std::max(4.0f, std::min(lim.ancho(), lim.profundidad()) * 0.055f);
        puntos.push_back(punto);
    }

    objetivoActual = puntos.empty() ? -1 : 0;
    std::cout << "[SCAN] " << puntos.size() << " zonas validas generadas\n";
}

void SistemaExploracion::reiniciar() {
    for (auto& punto : puntos) {
        punto.progreso = 0.0f;
        punto.estado = EstadoPuntoEscaneo::Pendiente;
    }
    completados = 0;
    objetivoActual = puntos.empty() ? -1 : 0;
    tiempoMision = 0.0f;
    distanciaRecorrida = 0.0f;
    tienePosicionAnterior = false;
}

int SistemaExploracion::buscarObjetivoMasCercano(const glm::vec3& posicion) const {
    int mejor = -1;
    float distanciaMejor = std::numeric_limits<float>::max();
    for (int i = 0; i < static_cast<int>(puntos.size()); ++i) {
        if (puntos[i].estado == EstadoPuntoEscaneo::Completado) continue;
        float d = glm::length(glm::vec2(posicion.x - puntos[i].posicion.x,
                                        posicion.z - puntos[i].posicion.z));
        if (d < distanciaMejor) { distanciaMejor = d; mejor = i; }
    }
    return mejor;
}

void SistemaExploracion::actualizar(const glm::vec3& posicionDron, float velocidadDron,
                                    float dt, MapaExploracion& mapa) {
    if (dt <= 0.0f || puntos.empty()) return;
    tiempoMision += dt;
    if (tienePosicionAnterior)
        distanciaRecorrida += glm::length(posicionDron - posicionAnterior);
    posicionAnterior = posicionDron;
    tienePosicionAnterior = true;

    objetivoActual = buscarObjetivoMasCercano(posicionDron);
    for (int i = 0; i < static_cast<int>(puntos.size()); ++i) {
        PuntoEscaneo& punto = puntos[i];
        if (punto.estado == EstadoPuntoEscaneo::Completado) continue;

        float distanciaHorizontal = glm::length(glm::vec2(posicionDron.x - punto.posicion.x,
                                                           posicionDron.z - punto.posicion.z));
        float alturaRelativa = posicionDron.y - punto.posicion.y;
        bool dentro = distanciaHorizontal <= punto.radio && alturaRelativa >= 1.5f && alturaRelativa <= 28.0f;
        bool estable = velocidadDron <= VELOCIDAD_MAX_ESCANEO;

        if (dentro && estable) {
            punto.estado = EstadoPuntoEscaneo::Escaneando;
            punto.progreso = std::min(1.0f, punto.progreso + dt / DURACION_ESCANEO);
        } else {
            punto.progreso = std::max(0.0f, punto.progreso - dt / (DURACION_ESCANEO * 2.0f));
            punto.estado = distanciaHorizontal <= RADIO_CERCANO
                         ? EstadoPuntoEscaneo::Cercano : EstadoPuntoEscaneo::Pendiente;
        }

        if (punto.progreso >= 1.0f) {
            punto.estado = EstadoPuntoEscaneo::Completado;
            ++completados;
            mapa.marcarZona(punto.posicion.x, punto.posicion.z, punto.radio * 2.5f);
            std::cout << "[SCAN] Punto " << punto.id << " completado ("
                      << completados << "/" << puntos.size() << ")\n";
        }
    }
    objetivoActual = buscarObjetivoMasCercano(posicionDron);
}

float SistemaExploracion::obtenerProgresoPuntos() const {
    if (puntos.empty()) return 0.0f;
    float progreso = 0.0f;
    for (const auto& punto : puntos) progreso += punto.progreso;
    return progreso / static_cast<float>(puntos.size());
}

float SistemaExploracion::distanciaAlObjetivo(const glm::vec3& posicion) const {
    if (objetivoActual < 0 || objetivoActual >= static_cast<int>(puntos.size())) return 0.0f;
    return glm::length(posicion - puntos[objetivoActual].posicion);
}
