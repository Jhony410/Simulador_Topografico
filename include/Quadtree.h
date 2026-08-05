#pragma once
#include "AABB.h"
#include "Limites.h"
#include "MallaCruda.h"

#include <vector>

class Frustum;

// ============================================================================
//  MODELO: Quadtree sobre la rejilla del terreno (nivel de detalle + descarte).
//
//  JUSTIFICACION DE LA ESTRUCTURA DE DATOS
//  ---------------------------------------
//  El terreno es una superficie REGULAR extendida en el plano XZ: su relieve
//  varia en Y, pero su ocupacion es la de una sabana plana. Un Quadtree divide
//  exactamente en ese plano (cuatro cuadrantes por nivel) y por eso encaja
//  mucho mejor que un Octree, cuya subdivision en Y produciria monton de nodos
//  vacios encima y debajo del relieve.
//
//  Sirve para dos cosas a la vez en un unico recorrido:
//    1) DESCARTE: si el AABB de un nodo queda fuera del frustum, se poda el
//       subarbol entero. Un nodo de nivel 1 descartado ahorra de golpe la
//       cuarta parte del mapa.
//    2) NIVEL DE DETALLE: cuanto mas lejos esta un cuadrante del dron, antes se
//       deja de descender, y los nodos poco profundos guardan indices generados
//       con un paso de muestreo mayor (1, 2, 4 u 8 vertices). Lo lejano se
//       dibuja con menos lineas sin que se note.
//
//  Los nodos viven en un std::vector y se referencian por INDICE, no por
//  puntero: la memoria queda contigua (mejor para la cache durante el
//  recorrido) y el arbol entero se libera de una sola vez.
// ============================================================================
class Quadtree {
public:
    struct Nodo {
        AABB caja;
        int  hijos[4] = {-1, -1, -1, -1};   // -1 en las hojas
        int  profundidad = 0;
        int  paso = 1;                      // submuestreo de sus indices
        float lado = 0.0f;                  // tamaño en unidades de mundo
        unsigned int offsetIndices = 0;     // en elementos, dentro de 'indices'
        unsigned int numIndices = 0;
    };

    // Construye el arbol a partir de la rejilla ya generada por el Terreno.
    // 'resolucion' es el numero de VERTICES por lado (celdas = resolucion - 1).
    void construir(const MallaCruda& rejilla, int resolucion, const LimitesMundo& limites);
    void limpiar();

    // Recorre podando por frustum y eligiendo profundidad segun la distancia al
    // dron. Deja en 'salida' los indices de los nodos que hay que dibujar.
    void seleccionar(const Frustum& frustum, const glm::vec3& posicionDron,
                     std::vector<int>& salida) const;

    const std::vector<Nodo>&         obtenerNodos()   const { return nodos; }
    const std::vector<unsigned int>& obtenerIndices() const { return indices; }
    bool vacio() const { return nodos.empty(); }
    std::size_t numeroNodos() const { return nodos.size(); }

private:
    int construirNodo(const MallaCruda& rejilla, int resolucion,
                      int x0, int z0, int lado, int profundidad);
    void recorrer(int indiceNodo, const Frustum& frustum,
                  const glm::vec3& posicionDron, std::vector<int>& salida) const;

    std::vector<Nodo>         nodos;
    std::vector<unsigned int> indices;   // EBO unico: cada nodo apunta a su tramo
};
