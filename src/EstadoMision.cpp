#include "EstadoMision.h"
#include "Configuracion.h"

#include <algorithm>

void EstadoMision::reiniciar() {
    progreso   = 0.0f;
    completada = false;
    alphaPanel = 0.0f;
}

void EstadoMision::actualizar(float porcentajeExplorado, float dt) {
    progreso = std::clamp(porcentajeExplorado, 0.0f, 1.0f);

    // Umbral por debajo del 100% exacto: barrer las ultimas celdas sueltas de
    // las esquinas es cuestion de suerte y dejaria la mision inacabable.
    if (!completada && progreso >= Configuracion::UMBRAL_MISION_COMPLETA)
        completada = true;

    if (completada) {
        alphaPanel = std::min(1.0f, alphaPanel + dt / Configuracion::DURACION_FADE_PANEL);
    }
}

int EstadoMision::obtenerPorcentaje() const {
    return (int)(obtenerProgreso() * 100.0f + 0.5f);
}
