#include "GrafoCurvas.h"

#include <cmath>

namespace {
// Empaqueta dos coordenadas de cubo en una sola clave de 64 bits.
uint64_t claveCubo(int cx, int cy) {
    return ((uint64_t)(uint32_t)cx << 32) | (uint64_t)(uint32_t)cy;
}
} // namespace

void GrafoCurvas::limpiar() {
    nodos.clear();
    aristas.clear();
    tablaEspacial.clear();
}

void GrafoCurvas::establecerEpsilon(float nuevoEpsilon) {
    epsilon = (nuevoEpsilon > 1e-6f) ? nuevoEpsilon : 1e-6f;
}

int GrafoCurvas::obtenerOCrearNodo(const glm::vec2& punto) {
    int cx = (int)std::floor(punto.x / epsilon);
    int cy = (int)std::floor(punto.y / epsilon);

    // Se miran los 9 cubos: un punto puede caer justo al borde de su cubo y
    // tener a su gemelo en el de al lado.
    for (int dy = -1; dy <= 1; ++dy) {
        for (int dx = -1; dx <= 1; ++dx) {
            auto it = tablaEspacial.find(claveCubo(cx + dx, cy + dy));
            if (it == tablaEspacial.end()) continue;
            for (int indice : it->second) {
                glm::vec2 d = nodos[indice].posicion - punto;
                if (d.x * d.x + d.y * d.y <= epsilon * epsilon) return indice;
            }
        }
    }

    int nuevo = (int)nodos.size();
    nodos.push_back(Nodo{punto, {}});
    tablaEspacial[claveCubo(cx, cy)].push_back(nuevo);
    return nuevo;
}

void GrafoCurvas::agregarSegmento(const glm::vec2& a, const glm::vec2& b) {
    int na = obtenerOCrearNodo(a);
    int nb = obtenerOCrearNodo(b);
    if (na == nb) return;   // segmento degenerado: no aporta nada a la curva

    int indice = (int)aristas.size();
    aristas.push_back(Arista{na, nb, false});
    nodos[na].aristas.push_back(indice);
    nodos[nb].aristas.push_back(indice);
}

int GrafoCurvas::primeraAristaLibre(int nodo) const {
    for (int indice : nodos[nodo].aristas)
        if (!aristas[indice].usada) return indice;
    return -1;
}

void GrafoCurvas::recorrerDesde(int inicio, float altura, std::vector<Polilinea>& salida) {
    Polilinea curva;
    curva.altura = altura;
    curva.puntos.push_back(nodos[inicio].posicion);

    int actual = inicio;
    while (true) {
        int indice = primeraAristaLibre(actual);
        if (indice < 0) break;                 // se acabo la cadena

        aristas[indice].usada = true;          // cada arista se consume una vez
        const Arista& arista = aristas[indice];
        int siguiente = (arista.a == actual) ? arista.b : arista.a;

        curva.puntos.push_back(nodos[siguiente].posicion);
        actual = siguiente;
        if (actual == inicio) break;           // volvimos al origen: ciclo cerrado
    }

    curva.cerrada = (actual == inicio && curva.puntos.size() > 2);
    if (curva.puntos.size() >= 2) salida.push_back(std::move(curva));
}

void GrafoCurvas::extraerPolilineas(float altura, std::vector<Polilinea>& salida) {
    for (auto& arista : aristas) arista.usada = false;

    // 1) Cadenas ABIERTAS. Arrancan en nodos de grado impar, que son los
    //    extremos sueltos (borde del mapa o frontera de la zona explorada).
    //    Hay que agotarlas primero: si se empezara por el medio, la cadena se
    //    partiria en dos trozos en vez de salir entera.
    for (int n = 0; n < (int)nodos.size(); ++n) {
        if (nodos[n].aristas.size() % 2 == 0) continue;
        while (primeraAristaLibre(n) >= 0) recorrerDesde(n, altura, salida);
    }

    // 2) Lo que sobra tiene todos los nodos de grado par: son ciclos CERRADOS.
    for (int n = 0; n < (int)nodos.size(); ++n) {
        while (primeraAristaLibre(n) >= 0) recorrerDesde(n, altura, salida);
    }
}
