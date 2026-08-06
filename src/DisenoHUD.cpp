#include "DisenoHUD.h"
#include "VistaMinimapa.h"

namespace DisenoHUD {

glm::vec4 rectBarraProgreso(float anchoPantalla, float altoPantalla) {
    // Se ancla al minimapa en vez de a una constante suelta: si el minimapa
    // cambia de sitio o de tamano, la barra lo sigue sin tocar nada mas.
    glm::vec4 mapa = VistaMinimapa::rectangulo(anchoPantalla, altoPantalla);
    const float alto = 2.0f;
    return glm::vec4(mapa.x, mapa.y + mapa.w + 26.0f, mapa.z, alto);
}

glm::vec4 rectControles(float anchoPantalla, float altoPantalla, int lineas) {
    // Ancho fijado por la accion mas larga a escala 1.05 ("GIRAR CAMARA
    // (ARRASTRAR)") mas la columna de teclas.
    const float ancho = 250.0f;
    const float altoLinea = 15.0f;
    const float alto = lineas * altoLinea;
    return glm::vec4(anchoPantalla - ancho - 32.0f, altoPantalla - alto - 30.0f, ancho, alto);
}

glm::vec4 rectPanelDebug(float anchoPantalla, float altoPantalla, int lineas) {
    (void)altoPantalla;
    const float ancho = 268.0f;
    const float alto = lineas * 15.0f + 20.0f;
    return glm::vec4(anchoPantalla - ancho - 24.0f, 24.0f, ancho, alto);
}

bool puntoDentro(double px, double py, const glm::vec4& rect) {
    return px >= rect.x && px <= rect.x + rect.z &&
           py >= rect.y && py <= rect.y + rect.w;
}

} // namespace DisenoHUD
