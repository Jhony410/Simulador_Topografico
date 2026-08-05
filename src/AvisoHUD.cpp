#include "AvisoHUD.h"

#include <algorithm>

namespace {
// Fraccion final del tiempo en la que el aviso se apaga.
constexpr float TRAMO_FUNDIDO = 0.35f;
}

void AvisoHUD::mostrar(std::string nuevoTexto, float duracion) {
    texto = std::move(nuevoTexto);
    duracionTotal = std::max(0.01f, duracion);
    restante = duracionTotal;
}

void AvisoHUD::actualizar(float dt) {
    if (restante > 0.0f) restante = std::max(0.0f, restante - dt);
}

void AvisoHUD::ocultar() {
    restante = 0.0f;
}

float AvisoHUD::obtenerAlpha() const {
    if (restante <= 0.0f) return 0.0f;
    float umbral = duracionTotal * TRAMO_FUNDIDO;
    if (restante >= umbral) return 1.0f;
    return restante / umbral;
}
