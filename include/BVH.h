#pragma once
#include "AABB.h"

#include <vector>

class Frustum;

// ============================================================================
//  MODELO: BVH (Bounding Volume Hierarchy) sobre objetos sueltos.
//
//  JUSTIFICACION DE LA ESTRUCTURA DE DATOS, Y POR QUE NO OTRO QUADTREE
//  ------------------------------------------------------------------
//  El Quadtree parte el ESPACIO en cuadrantes fijos; sirve para el terreno,
//  que llena ese espacio de forma regular. Las estacas de sondeo son lo
//  contrario: 120 objetos dispersos, a alturas muy distintas y con zonas del
//  mapa vacias. Un Quadtree sobre ellas generaria cuadrantes sin un solo
//  objeto, y las estacas altas obligarian a estirar los nodos en Y.
//
//  El BVH parte los OBJETOS, no el espacio: cada nodo envuelve exactamente el
//  subconjunto que contiene, sin huecos ni solapes inutiles. Se construye
//  dividiendo por la MEDIANA del eje mas largo, lo que da un arbol equilibrado
//  de profundidad O(log n) y, por tanto, descarte en O(log n) en vez de recorrer
//  las 120 cajas una por una.
//
//  Igual que el Quadtree, los nodos van en un vector y se enlazan por indice.
// ============================================================================
class BVH {
public:
    struct Nodo {
        AABB caja;
        int  izquierdo = -1, derecho = -1;   // -1 -> es hoja
        int  primerObjeto = 0, numObjetos = 0;   // tramo dentro de 'orden'
    };

    // 'cajas' son los volumenes de cada objeto, en el orden original.
    void construir(const std::vector<AABB>& cajas);
    void limpiar();

    // Deja en 'visibles' los INDICES ORIGINALES de los objetos no descartados.
    void consultar(const Frustum& frustum, std::vector<int>& visibles) const;

    bool vacio() const { return nodos.empty(); }
    std::size_t numeroNodos() const { return nodos.size(); }

private:
    int construirNodo(const std::vector<AABB>& cajas, int inicio, int fin);
    void recorrer(int indiceNodo, const Frustum& frustum, std::vector<int>& visibles) const;

    std::vector<Nodo> nodos;
    std::vector<int>  orden;   // permutacion de indices de objeto por hojas
};
