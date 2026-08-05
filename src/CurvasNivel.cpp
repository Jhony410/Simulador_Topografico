#include "CurvasNivel.h"

#include <algorithm>

void CurvasNivel::limpiar() {
    curvas.clear();
}

void CurvasNivel::agregar(Polilinea curva) {
    curvas.push_back(std::move(curva));
}

void CurvasNivel::establecerRango(float minimo, float maximo) {
    alturaMinima = minimo;
    alturaMaxima = maximo;
}

float CurvasNivel::normalizar(float altura) const {
    float rango = alturaMaxima - alturaMinima;
    if (rango < 1e-6f) return 0.0f;
    return std::clamp((altura - alturaMinima) / rango, 0.0f, 1.0f);
}
