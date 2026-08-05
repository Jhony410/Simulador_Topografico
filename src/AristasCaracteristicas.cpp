#include "AristasCaracteristicas.h"

#include <cmath>
#include <cstdint>
#include <glm/glm.hpp>
#include <unordered_map>

namespace {

// Clave de rejilla para soldar posiciones. Los duplicados que generan los
// exportadores tienen coordenadas IDENTICAS bit a bit, asi que cuantizar y
// mirar un solo cubo basta: no hace falta explorar los vecinos.
uint64_t claveDePosicion(const glm::vec3& p, float epsilon) {
    int64_t x = (int64_t)std::floor(p.x / epsilon);
    int64_t y = (int64_t)std::floor(p.y / epsilon);
    int64_t z = (int64_t)std::floor(p.z / epsilon);
    // Mezcla de los tres ejes en 64 bits (constantes primas grandes).
    return (uint64_t)(x * 73856093LL) ^ (uint64_t)(y * 19349663LL) ^ (uint64_t)(z * 83492791LL);
}

// Clave de arista: par ordenado de indices soldados.
uint64_t claveDeArista(uint32_t a, uint32_t b) {
    if (a > b) std::swap(a, b);
    return ((uint64_t)a << 32) | (uint64_t)b;
}

struct DatosArista {
    glm::vec3 normalPrimera{0.0f};
    unsigned int origenA = 0, origenB = 0;   // indices en el arreglo ORIGINAL
    int caras = 0;
    bool marcada = false;
};

} // namespace

namespace AristasCaracteristicas {

std::vector<unsigned int> extraer(const std::vector<float>& vertices,
                                  int floatsPorVertice,
                                  const std::vector<unsigned int>& indices,
                                  float anguloGrados,
                                  float epsilonSoldadura) {
    std::vector<unsigned int> salida;
    if (floatsPorVertice < 3 || vertices.empty() || indices.size() < 3) return salida;

    const std::size_t numVertices = vertices.size() / floatsPorVertice;
    auto posicionDe = [&](std::size_t i) {
        std::size_t base = i * floatsPorVertice;
        return glm::vec3(vertices[base], vertices[base + 1], vertices[base + 2]);
    };

    // ---- 1) Soldadura por posicion ----
    std::unordered_map<uint64_t, uint32_t> tabla;
    tabla.reserve(numVertices * 2);
    std::vector<uint32_t> soldado(numVertices);
    uint32_t siguienteSoldado = 0;
    for (std::size_t i = 0; i < numVertices; ++i) {
        uint64_t clave = claveDePosicion(posicionDe(i), epsilonSoldadura);
        auto it = tabla.find(clave);
        if (it == tabla.end()) {
            tabla.emplace(clave, siguienteSoldado);
            soldado[i] = siguienteSoldado++;
        } else {
            soldado[i] = it->second;
        }
    }

    // ---- 2) Recorrer caras acumulando adyacencia por arista ----
    std::unordered_map<uint64_t, DatosArista> aristas;
    aristas.reserve(indices.size());
    const float cosUmbral = std::cos(glm::radians(anguloGrados));

    for (std::size_t t = 0; t + 2 < indices.size(); t += 3) {
        unsigned int i0 = indices[t], i1 = indices[t + 1], i2 = indices[t + 2];
        if (i0 >= numVertices || i1 >= numVertices || i2 >= numVertices) continue;

        glm::vec3 p0 = posicionDe(i0), p1 = posicionDe(i1), p2 = posicionDe(i2);
        glm::vec3 normal = glm::cross(p1 - p0, p2 - p0);
        float longitud = glm::length(normal);
        if (longitud < 1e-12f) continue;          // triangulo degenerado
        normal /= longitud;

        const unsigned int tri[3] = {i0, i1, i2};
        for (int k = 0; k < 3; ++k) {
            unsigned int oa = tri[k], ob = tri[(k + 1) % 3];
            uint32_t sa = soldado[oa], sb = soldado[ob];
            if (sa == sb) continue;               // arista degenerada tras soldar

            DatosArista& d = aristas[claveDeArista(sa, sb)];
            if (d.caras == 0) {
                d.normalPrimera = normal;
                d.origenA = oa;
                d.origenB = ob;
            } else if (d.caras == 1) {
                // Angulo diedro: si las dos caras forman un canto, es arista real.
                if (glm::dot(d.normalPrimera, normal) < cosUmbral) d.marcada = true;
            } else {
                // Mas de dos caras: geometria no-manifold, se conserva siempre.
                d.marcada = true;
            }
            ++d.caras;
        }
    }

    // ---- 3) Volcar pliegues y bordes ----
    salida.reserve(aristas.size());
    for (const auto& par : aristas) {
        const DatosArista& d = par.second;
        // caras == 1 -> borde abierto de la malla.
        if (d.marcada || d.caras == 1) {
            salida.push_back(d.origenA);
            salida.push_back(d.origenB);
        }
    }
    return salida;
}

} // namespace AristasCaracteristicas
