#pragma once
#include "CurvasNivel.h"

#include <cstdint>
#include <glm/glm.hpp>
#include <unordered_map>
#include <vector>

// ============================================================================
//  MODELO: GRAFO de encadenado de isolineas.
//
//  JUSTIFICACION DE LA ESTRUCTURA DE DATOS
//  ---------------------------------------
//  Marching Squares devuelve segmentos SUELTOS y en desorden: cada celda emite
//  su trocito sin saber nada de sus vecinas. Para dibujar curvas continuas hay
//  que descubrir que trocitos comparten extremo y en que orden se encadenan.
//  Eso es exactamente un grafo no dirigido:
//      NODOS   = puntos de interseccion (extremos de los segmentos)
//      ARISTAS = los segmentos
//  y recorrerlo consumiendo aristas produce las polilineas.
//
//  ¿Por que un grafo y no ordenar los segmentos?
//   - No existe un orden lineal valido: en un punto de silla concurren CUATRO
//     segmentos, asi que la estructura se ramifica. Solo un grafo lo modela.
//   - Ordenar por proximidad seria O(n^2) y ademas fragil: dos curvas distintas
//     que pasan cerca se pegarian.
//   - Con listas de adyacencia el recorrido es O(V + E): cada arista se visita
//     una sola vez.
//
//  Fusion de extremos: dos celdas vecinas calculan el cruce de su arista comun
//  con la MISMA formula, asi que los puntos deberian coincidir bit a bit; aun
//  asi se unen con una TABLA HASH ESPACIAL (unordered_map de celda -> nodos)
//  que acepta coincidencias dentro de un epsilon. Buscar el nodo existente en
//  un vector seria O(n) por consulta; la tabla lo resuelve en O(1) promedio
//  mirando solo el cubo del punto y sus 8 vecinos.
// ============================================================================
class GrafoCurvas {
public:
    void limpiar();

    // Tolerancia con la que dos extremos se consideran el mismo nodo.
    void establecerEpsilon(float epsilon);

    void agregarSegmento(const glm::vec2& a, const glm::vec2& b);

    // Consume las aristas y vuelca las polilineas resultantes en 'salida'.
    void extraerPolilineas(float altura, std::vector<Polilinea>& salida);

    std::size_t numeroNodos()   const { return nodos.size(); }
    std::size_t numeroAristas() const { return aristas.size(); }

private:
    struct Nodo {
        glm::vec2 posicion{0.0f};
        std::vector<int> aristas;   // lista de adyacencia (indices a 'aristas')
    };
    struct Arista {
        int  a = -1, b = -1;
        bool usada = false;
    };

    int  obtenerOCrearNodo(const glm::vec2& punto);
    int  primeraAristaLibre(int nodo) const;
    void recorrerDesde(int inicio, float altura, std::vector<Polilinea>& salida);

    std::vector<Nodo>   nodos;
    std::vector<Arista> aristas;
    std::unordered_map<uint64_t, std::vector<int>> tablaEspacial;
    float epsilon = 0.02f;
};
