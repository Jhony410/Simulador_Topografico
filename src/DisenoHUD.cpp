#include "DisenoHUD.h"

namespace DisenoHUD {

glm::vec4 rectBotonMapa(int indice, float anchoPantalla, float altoPantalla) {
    const float ancho = 40.0f, alto = 40.0f, separacion = 8.0f;
    const int   total = 5;   // ancho de referencia del bloque para centrarlo
    float bloque = total * ancho + (total - 1) * separacion;
    // Centrados en el borde inferior: la esquina inferior izquierda queda libre
    // para el pie de proyecto, como en la referencia.
    float x0 = (anchoPantalla - bloque) * 0.5f;
    return glm::vec4(x0 + indice * (ancho + separacion), altoPantalla - alto - 26.0f, ancho, alto);
}

glm::vec4 rectTecla(int indice, float anchoPantalla, float altoPantalla) {
    const float ancho = 46.0f, alto = 38.0f, separacion = 7.0f;
    float baseX = anchoPantalla - (ancho * 3 + separacion * 2) - 26.0f;

    // De abajo hacia arriba: SHIFT/SPACE, luego la fila de flechas laterales,
    // y la flecha de subir centrada encima de todo.
    float filaModificadores = altoPantalla - 26.0f - alto;
    float filaFlechas       = filaModificadores - alto - separacion;
    float filaArriba        = filaFlechas - alto - separacion;

    const float anchoModificador = (ancho * 3 + separacion * 2 - separacion) * 0.5f;

    switch (indice) {
        case 0: return glm::vec4(baseX + ancho + separacion,       filaArriba,  ancho, alto);
        case 1: return glm::vec4(baseX,                            filaFlechas, ancho, alto);
        case 2: return glm::vec4(baseX + ancho + separacion,        filaFlechas, ancho, alto);
        case 3: return glm::vec4(baseX + 2 * (ancho + separacion),  filaFlechas, ancho, alto);
        case 4: return glm::vec4(baseX,                             filaModificadores, anchoModificador, alto);
        case 5: return glm::vec4(baseX + anchoModificador + separacion, filaModificadores, anchoModificador, alto);
        default: return glm::vec4(0.0f);
    }
}

glm::vec4 rectBarraProgreso(float anchoPantalla, float altoPantalla) {
    (void)anchoPantalla;
    const float ancho = 260.0f, alto = 3.0f;
    // Columna izquierda: panel de curvas arriba, barra en medio, pie abajo.
    return glm::vec4(24.0f, altoPantalla - 96.0f, ancho, alto);
}

glm::vec4 rectPanelCurvas(float anchoPantalla, float altoPantalla) {
    (void)anchoPantalla;
    const float ancho = 330.0f, alto = 215.0f;
    return glm::vec4(24.0f, altoPantalla - 118.0f - alto, ancho, alto);
}

glm::vec4 rectPanelMision(float anchoPantalla, float altoPantalla) {
    (void)altoPantalla;
    const float ancho = 660.0f, alto = 70.0f;
    return glm::vec4((anchoPantalla - ancho) * 0.5f, 26.0f, ancho, alto);
}

glm::vec4 rectPanelLateral(float anchoPantalla, float altoPantalla) {
    // El ancho lo fija el parrafo: a escala 1.35 la linea mas larga mide ~280 px
    // y necesita los 22 px de margen a cada lado.
    const float ancho = 390.0f, alto = 210.0f;
    return glm::vec4(anchoPantalla - ancho - 46.0f, (altoPantalla - alto) * 0.5f, ancho, alto);
}

glm::vec4 rectBotonSalir(float anchoPantalla, float altoPantalla) {
    (void)altoPantalla;
    const float lado = 44.0f;
    return glm::vec4(anchoPantalla - lado - 30.0f, 26.0f, lado, lado);
}

glm::vec4 rectIconoMenu(float anchoPantalla, float altoPantalla) {
    (void)altoPantalla;
    const float ancho = 26.0f, alto = 18.0f;
    return glm::vec4(anchoPantalla - ancho - 39.0f, 86.0f, ancho, alto);
}

bool puntoDentro(double px, double py, const glm::vec4& rect) {
    return px >= rect.x && px <= rect.x + rect.z &&
           py >= rect.y && py <= rect.y + rect.w;
}

} // namespace DisenoHUD
