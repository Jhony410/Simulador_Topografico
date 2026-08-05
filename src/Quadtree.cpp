#include "Quadtree.h"
#include "Configuracion.h"
#include "Frustum.h"

#include <algorithm>

namespace {
// Paso de muestreo por profundidad. Los nodos poco profundos son los grandes,
// y solo se dibujan enteros cuando estan LEJOS: por eso les toca el paso mas
// basto. Los profundos son los que quedan bajo el dron y van a paso 1.
int pasoDeProfundidad(int profundidad) {
    switch (profundidad) {
        case 0: return 8;
        case 1: return 8;
        case 2: return 4;
        case 3: return 2;
        default: return 1;
    }
}
} // namespace

void Quadtree::limpiar() {
    nodos.clear();
    indices.clear();
}

void Quadtree::construir(const MallaCruda& rejilla, int resolucion, const LimitesMundo& limites) {
    (void)limites;
    limpiar();
    int celdas = resolucion - 1;
    if (resolucion < 2 || rejilla.vertices.empty()) return;

    // El lado en celdas debe ser divisible entre 2^profundidad para que los
    // cuadrantes caigan siempre sobre vertices existentes.
    int divisor = 1 << Configuracion::PROFUNDIDAD_QUADTREE;
    if (celdas % divisor != 0) return;

    nodos.reserve(1365);   // 1 + 4 + 16 + 64 + 256 + 1024 para profundidad 5
    construirNodo(rejilla, resolucion, 0, 0, celdas, 0);
}

int Quadtree::construirNodo(const MallaCruda& rejilla, int resolucion,
                            int x0, int z0, int lado, int profundidad) {
    int indiceNodo = (int)nodos.size();
    nodos.push_back(Nodo{});

    // --- AABB: recorre los vertices del cuadrante y encierra su relieve ---
    AABB caja;
    for (int z = z0; z <= z0 + lado; ++z) {
        for (int x = x0; x <= x0 + lado; ++x) {
            std::size_t v = ((std::size_t)z * resolucion + x) * 3;
            caja.expandir(glm::vec3(rejilla.vertices[v], rejilla.vertices[v + 1],
                                    rejilla.vertices[v + 2]));
        }
    }

    int paso = pasoDeProfundidad(profundidad);
    paso = std::min(paso, std::max(1, lado));   // nunca mayor que el propio nodo

    // --- Indices propios del nodo, generados a su paso de muestreo ---
    unsigned int offset = (unsigned int)indices.size();
    for (int z = z0; z <= z0 + lado; z += paso)
        for (int x = x0; x + paso <= x0 + lado; x += paso) {
            indices.push_back((unsigned int)(z * resolucion + x));
            indices.push_back((unsigned int)(z * resolucion + x + paso));
        }
    for (int z = z0; z + paso <= z0 + lado; z += paso)
        for (int x = x0; x <= x0 + lado; x += paso) {
            indices.push_back((unsigned int)(z * resolucion + x));
            indices.push_back((unsigned int)((z + paso) * resolucion + x));
        }
    // La cuenta se cierra AQUI, antes de recursar: los hijos siguen añadiendo
    // al mismo vector, y medirla despues haria que cada nodo abarcase tambien
    // los indices de todo su subarbol.
    unsigned int cuenta = (unsigned int)indices.size() - offset;

    // --- Hijos ---
    int hijos[4] = {-1, -1, -1, -1};
    if (profundidad < Configuracion::PROFUNDIDAD_QUADTREE && lado >= 2) {
        int mitad = lado / 2;
        hijos[0] = construirNodo(rejilla, resolucion, x0,         z0,         mitad, profundidad + 1);
        hijos[1] = construirNodo(rejilla, resolucion, x0 + mitad, z0,         mitad, profundidad + 1);
        hijos[2] = construirNodo(rejilla, resolucion, x0,         z0 + mitad, mitad, profundidad + 1);
        hijos[3] = construirNodo(rejilla, resolucion, x0 + mitad, z0 + mitad, mitad, profundidad + 1);
    }

    // Se escribe al final: los hijos han podido reasignar el vector 'nodos' y
    // cualquier referencia tomada antes estaria colgando.
    Nodo& nodo = nodos[indiceNodo];
    nodo.caja = caja;
    nodo.profundidad = profundidad;
    nodo.paso = paso;
    nodo.lado = std::max(caja.extension().x, caja.extension().z);
    nodo.offsetIndices = offset;
    nodo.numIndices = cuenta;
    for (int i = 0; i < 4; ++i) nodo.hijos[i] = hijos[i];
    return indiceNodo;
}

void Quadtree::seleccionar(const Frustum& frustum, const glm::vec3& posicionDron,
                           std::vector<int>& salida) const {
    if (nodos.empty()) return;
    recorrer(0, frustum, posicionDron, salida);
}

void Quadtree::recorrer(int indiceNodo, const Frustum& frustum,
                        const glm::vec3& posicionDron, std::vector<int>& salida) const {
    const Nodo& nodo = nodos[indiceNodo];

    // 1) Descarte: fuera del frustum se poda el subarbol completo.
    if (!frustum.intersecta(nodo.caja)) return;

    bool esHoja = (nodo.hijos[0] < 0);
    // 2) Nivel de detalle: si el nodo esta lo bastante lejos en proporcion a su
    //    tamaño, se dibuja tal cual (con su paso basto) y no se desciende mas.
    float distancia = nodo.caja.distanciaA(posicionDron);
    if (esHoja || distancia > Configuracion::FACTOR_LOD * nodo.lado) {
        if (nodo.numIndices > 0) salida.push_back(indiceNodo);
        return;
    }

    for (int i = 0; i < 4; ++i) recorrer(nodo.hijos[i], frustum, posicionDron, salida);
}
