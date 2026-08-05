#include "BVH.h"
#include "Frustum.h"

#include <algorithm>

namespace {
// Por debajo de este numero de objetos no compensa seguir partiendo: el coste
// de bajar un nivel supera al de probar las cajas una a una.
constexpr int OBJETOS_POR_HOJA = 4;
} // namespace

void BVH::limpiar() {
    nodos.clear();
    orden.clear();
}

void BVH::construir(const std::vector<AABB>& cajas) {
    limpiar();
    if (cajas.empty()) return;

    orden.resize(cajas.size());
    for (int i = 0; i < (int)cajas.size(); ++i) orden[i] = i;

    nodos.reserve(cajas.size() * 2);
    construirNodo(cajas, 0, (int)cajas.size());
}

int BVH::construirNodo(const std::vector<AABB>& cajas, int inicio, int fin) {
    int indiceNodo = (int)nodos.size();
    nodos.push_back(Nodo{});

    AABB caja;
    for (int i = inicio; i < fin; ++i) caja.expandir(cajas[orden[i]]);

    int cantidad = fin - inicio;
    int izquierdo = -1, derecho = -1;

    if (cantidad > OBJETOS_POR_HOJA) {
        // Se parte por el eje mas largo de la caja del nodo: es el que mas
        // separa los objetos y por tanto el que deja hijos menos solapados.
        glm::vec3 extension = caja.extension();
        int eje = 0;
        if (extension.y > extension.x) eje = 1;
        if (extension.z > extension[eje]) eje = 2;

        int medio = inicio + cantidad / 2;
        // nth_element deja la mediana en su sitio en O(n), sin ordenar el resto:
        // no necesitamos el orden completo, solo la particion.
        std::nth_element(orden.begin() + inicio, orden.begin() + medio, orden.begin() + fin,
                         [&cajas, eje](int a, int b) {
                             return cajas[a].centro()[eje] < cajas[b].centro()[eje];
                         });
        izquierdo = construirNodo(cajas, inicio, medio);
        derecho   = construirNodo(cajas, medio, fin);
    }

    // Al final: los hijos pueden haber reasignado el vector.
    Nodo& nodo = nodos[indiceNodo];
    nodo.caja = caja;
    nodo.izquierdo = izquierdo;
    nodo.derecho = derecho;
    nodo.primerObjeto = inicio;
    nodo.numObjetos = cantidad;
    return indiceNodo;
}

void BVH::consultar(const Frustum& frustum, std::vector<int>& visibles) const {
    if (nodos.empty()) return;
    recorrer(0, frustum, visibles);
}

void BVH::recorrer(int indiceNodo, const Frustum& frustum, std::vector<int>& visibles) const {
    const Nodo& nodo = nodos[indiceNodo];
    if (!frustum.intersecta(nodo.caja)) return;   // se poda el subarbol entero

    if (nodo.izquierdo < 0) {
        for (int i = 0; i < nodo.numObjetos; ++i)
            visibles.push_back(orden[nodo.primerObjeto + i]);
        return;
    }
    recorrer(nodo.izquierdo, frustum, visibles);
    recorrer(nodo.derecho,   frustum, visibles);
}
