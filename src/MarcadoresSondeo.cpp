#include "MarcadoresSondeo.h"
#include "Configuracion.h"
#include "EscalaMundo.h"
#include "Terreno.h"

#include <random>

void MarcadoresSondeo::limpiar() {
    marcadores.clear();
    cajas.clear();
    bvh.limpiar();
}

void MarcadoresSondeo::generar(const Terreno& terreno, const EscalaMundo& escala,
                               int cantidad, unsigned int semilla) {
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
        // La base es la SUPERFICIE REAL en ese (x, z): sin sumandos arbitrarios
        // y sin probabilidad de flotar. En una pendiente la torreta nace pegada
        // al relieve porque alturaEn() interpola bilinealmente el heightmap.
        m.base = glm::vec3(x, terreno.alturaEn(x, z), z);

        // Variacion moderada alrededor de la altura nominal de la escala, para
        // que el campo de torretas no parezca clonado.
        float variacion = 1.0f + (aleatorio() * 2.0f - 1.0f) * Configuracion::MARCADOR_VARIACION;
        m.altura = escala.alturaMarcador * variacion;
        m.ancho  = escala.anchoMarcador;
        marcadores.push_back(m);
    }

    // Caja de cada torreta: el mastil mas su propio ancho a los lados.
    cajas.reserve(marcadores.size());
    for (const auto& m : marcadores) {
        AABB caja;
        caja.expandir(m.base - glm::vec3(m.ancho, 0.0f, m.ancho));
        caja.expandir(m.base + glm::vec3(m.ancho, m.altura + m.ancho, m.ancho));
        cajas.push_back(caja);
    }
    bvh.construir(cajas);
}
