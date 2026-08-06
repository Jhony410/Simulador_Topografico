#include "MapaExploracion.h"

#include <algorithm>
#include <cmath>

void MapaExploracion::reiniciar(int nuevoAncho, int nuevoAlto, const LimitesMundo& nuevosLimites) {
    ancho = std::max(0, nuevoAncho);
    alto  = std::max(0, nuevoAlto);
    limites = nuevosLimites;
    mascara.assign(static_cast<std::size_t>(ancho) * alto, 0);
    celdasMarcadas = 0;
    ++revision;
}

void MapaExploracion::mundoACelda(float x, float z, int& cx, int& cz) const {
    if (ancho <= 0 || alto <= 0) { cx = cz = 0; return; }
    cx = (int)((x - limites.minX) / limites.ancho()       * (ancho - 1) + 0.5f);
    cz = (int)((z - limites.minZ) / limites.profundidad() * (alto  - 1) + 0.5f);
}

bool MapaExploracion::estaExplorada(int cx, int cz) const {
    if (cx < 0 || cz < 0 || cx >= ancho || cz >= alto) return false;
    return mascara[(std::size_t)cz * ancho + cx] != 0;
}

bool MapaExploracion::marcarCelda(int cx, int cz) {
    if (cx < 0 || cz < 0 || cx >= ancho || cz >= alto) return false;
    uint8_t& celda = mascara[(std::size_t)cz * ancho + cx];
    if (celda) return false;
    celda = 1;
    ++celdasMarcadas;   // contador incremental: evita recorrer todo el mapa
    ++revision;
    return true;
}

void MapaExploracion::marcarZona(float x, float z, float radio) {
    if (ancho <= 0 || alto <= 0) return;

    // Radio de mundo -> radio en celdas, por eje (la grilla no es cuadrada en mundo).
    float celdasPorX = (ancho - 1) / limites.ancho();
    float celdasPorZ = (alto  - 1) / limites.profundidad();
    int rx = (int)std::ceil(radio * celdasPorX);
    int rz = (int)std::ceil(radio * celdasPorZ);

    int cx, cz;
    mundoACelda(x, z, cx, cz);

    for (int dz = -rz; dz <= rz; ++dz) {
        for (int dx = -rx; dx <= rx; ++dx) {
            // Test elipsoidal en espacio de celdas = circulo en espacio de mundo.
            float nx = (rx > 0) ? (float)dx / rx : 0.0f;
            float nz = (rz > 0) ? (float)dz / rz : 0.0f;
            if (nx * nx + nz * nz > 1.0f) continue;
            marcarCelda(cx + dx, cz + dz);
        }
    }
}

std::vector<uint32_t> MapaExploracion::comprimirRLE() const {
    std::vector<uint32_t> tiradas;
    if (mascara.empty()) return tiradas;

    uint8_t valorActual = 0;   // se arranca contando celdas SIN explorar
    uint32_t longitud = 0;
    for (uint8_t celda : mascara) {
        uint8_t normalizado = celda ? 1 : 0;
        if (normalizado == valorActual) {
            ++longitud;
        } else {
            // Si el mapa empieza por celdas exploradas, la primera tirada vale
            // 0: es lo que mantiene la alternancia sin guardar el valor.
            tiradas.push_back(longitud);
            valorActual = normalizado;
            longitud = 1;
        }
    }
    tiradas.push_back(longitud);
    return tiradas;
}

bool MapaExploracion::descomprimirRLE(const std::vector<uint32_t>& tiradas,
                                      int nuevoAncho, int nuevoAlto) {
    if (nuevoAncho <= 0 || nuevoAlto <= 0) return false;
    // La grilla del guardado debe cuadrar con la del mapa ya cargado; si no,
    // el archivo pertenece a otra resolucion y no se puede aplicar.
    if (nuevoAncho != ancho || nuevoAlto != alto) return false;

    std::size_t total = (std::size_t)ancho * alto;
    std::size_t suma = 0;
    for (uint32_t t : tiradas) suma += t;
    if (suma != total) return false;   // archivo truncado o corrupto

    mascara.assign(total, 0);
    celdasMarcadas = 0;

    std::size_t posicion = 0;
    uint8_t valor = 0;
    for (uint32_t longitud : tiradas) {
        if (valor) {
            for (uint32_t k = 0; k < longitud; ++k) mascara[posicion + k] = 1;
            celdasMarcadas += longitud;
        }
        posicion += longitud;
        valor ^= 1;
    }
    ++revision;
    return true;
}

float MapaExploracion::porcentajeExplorado() const {
    std::size_t total = mascara.size();
    if (total == 0) return 0.0f;
    return (float)celdasMarcadas / (float)total;
}
