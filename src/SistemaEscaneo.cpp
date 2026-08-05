#include "SistemaEscaneo.h"
#include "MapaExploracion.h"

#include <algorithm>
#include <cmath>

void SistemaEscaneo::reiniciar(int nuevoAncho, int nuevoAlto) {
    ancho = std::max(0, nuevoAncho);
    alto  = std::max(0, nuevoAlto);
    enCola.assign(static_cast<std::size_t>(ancho) * alto, 0);
    // swap con una cola vacia es la forma canonica de liberar la memoria de
    // std::queue; clear() no existe en su interfaz.
    std::queue<uint32_t> vacia;
    cola.swap(vacia);
}

void SistemaEscaneo::detectar(const MapaExploracion& mapa, float x, float z, float radio) {
    if (ancho <= 0 || alto <= 0) return;

    const LimitesMundo& lim = mapa.obtenerLimites();
    // Radio de mundo -> radio en celdas, por eje: la grilla no tiene por que
    // ser cuadrada en unidades de mundo.
    float celdasPorX = (ancho - 1) / lim.ancho();
    float celdasPorZ = (alto  - 1) / lim.profundidad();
    int rx = (int)std::ceil(radio * celdasPorX);
    int rz = (int)std::ceil(radio * celdasPorZ);

    int cx, cz;
    mapa.mundoACelda(x, z, cx, cz);

    for (int dz = -rz; dz <= rz; ++dz) {
        for (int dx = -rx; dx <= rx; ++dx) {
            // Elipse en espacio de celdas = circulo en espacio de mundo.
            float nx = (rx > 0) ? (float)dx / rx : 0.0f;
            float nz = (rz > 0) ? (float)dz / rz : 0.0f;
            if (nx * nx + nz * nz > 1.0f) continue;

            int celdaX = cx + dx, celdaZ = cz + dz;
            if (celdaX < 0 || celdaZ < 0 || celdaX >= ancho || celdaZ >= alto) continue;

            std::size_t indice = (std::size_t)celdaZ * ancho + celdaX;
            if (mapa.estaExplorada(celdaX, celdaZ)) continue;
            if (enCola[indice]) continue;

            enCola[indice] = 1;
            cola.push((uint32_t)indice);
        }
    }
}

int SistemaEscaneo::procesarLote(MapaExploracion& mapa, int lote) {
    int marcadas = 0;
    for (int i = 0; i < lote && !cola.empty(); ++i) {
        uint32_t indice = cola.front();
        cola.pop();
        enCola[indice] = 0;

        int celdaZ = (int)(indice / ancho);
        int celdaX = (int)(indice % ancho);
        if (mapa.marcarCelda(celdaX, celdaZ)) ++marcadas;
    }
    return marcadas;
}
