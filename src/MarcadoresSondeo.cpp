#include "MarcadoresSondeo.h"
#include "Configuracion.h"
#include "Terreno.h"

#include <random>

void MarcadoresSondeo::limpiar() {
    marcadores.clear();
    cajas.clear();
    bvh.limpiar();
}

void MarcadoresSondeo::generar(const Terreno& terreno, int cantidad, unsigned int semilla) {
    limpiar();
    if (cantidad <= 0 || !terreno.estaCargado()) return;
    marcadores.reserve(cantidad);

    // Mersenne Twister sembrado a mano. No se usa uniform_real_distribution
    // porque su resultado no esta garantizado identico entre compiladores: al
    // normalizar a mano, la siembra es reproducible en cualquier maquina.
    std::mt19937 generador(semilla);
    auto aleatorio = [&generador]() {
        return (float)generador() / (float)std::mt19937::max();
    };

    const LimitesMundo& lim = terreno.obtenerLimites();
    // Margen para que ninguna estaca caiga justo en el borde del mapa.
    const float margenX = lim.ancho()       * 0.06f;
    const float margenZ = lim.profundidad() * 0.06f;

    for (int i = 0; i < cantidad; ++i) {
        float x = lim.minX + margenX + aleatorio() * (lim.ancho()       - 2.0f * margenX);
        float z = lim.minZ + margenZ + aleatorio() * (lim.profundidad() - 2.0f * margenZ);

        MarcadorSondeo m;
        m.base = glm::vec3(x, terreno.alturaEn(x, z), z);

        // Una parte de las estacas se despega del suelo: da sensacion de
        // sondeos aereos y rompe la monotonia de tenerlas todas pegadas.
        if (aleatorio() < Configuracion::MARCADOR_PROB_FLOTANTE)
            m.base.y += aleatorio() * Configuracion::MARCADOR_FLOTE_MAX;

        m.altura = Configuracion::MARCADOR_ALTURA_MIN +
                   aleatorio() * (Configuracion::MARCADOR_ALTURA_MAX - Configuracion::MARCADOR_ALTURA_MIN);
        marcadores.push_back(m);
    }

    // Caja de cada estaca: el poste mas un margen lateral para la cabeza, que
    // es un billboard y ocupa sitio en cualquier direccion.
    const float margen = Configuracion::MARCADOR_TAM_CABEZA;
    cajas.reserve(marcadores.size());
    for (const auto& m : marcadores) {
        AABB caja;
        caja.expandir(m.base - glm::vec3(margen, 0.0f, margen));
        caja.expandir(m.base + glm::vec3(margen, m.altura + margen, margen));
        cajas.push_back(caja);
    }
    bvh.construir(cajas);
}
