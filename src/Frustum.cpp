#include "Frustum.h"

#include <cmath>

void Frustum::extraerDe(const glm::mat4& m) {
    // GLM guarda las matrices por COLUMNAS: m[columna][fila]. Las filas de la
    // matriz combinada se reconstruyen a mano.
    glm::vec4 fila0(m[0][0], m[1][0], m[2][0], m[3][0]);
    glm::vec4 fila1(m[0][1], m[1][1], m[2][1], m[3][1]);
    glm::vec4 fila2(m[0][2], m[1][2], m[2][2], m[3][2]);
    glm::vec4 fila3(m[0][3], m[1][3], m[2][3], m[3][3]);

    planos[0] = fila3 + fila0;   // izquierdo
    planos[1] = fila3 - fila0;   // derecho
    planos[2] = fila3 + fila1;   // inferior
    planos[3] = fila3 - fila1;   // superior
    planos[4] = fila3 + fila2;   // cercano
    planos[5] = fila3 - fila2;   // lejano

    // Normalizar deja w como una distancia real, no como un valor a escala
    // arbitraria; sin esto el test seguiria dando el signo correcto pero no
    // se podria usar para medir separaciones.
    for (int i = 0; i < 6; ++i) {
        float longitud = std::sqrt(planos[i].x * planos[i].x +
                                   planos[i].y * planos[i].y +
                                   planos[i].z * planos[i].z);
        if (longitud > 1e-8f) planos[i] /= longitud;
    }
}

bool Frustum::intersecta(const AABB& caja) const {
    for (int i = 0; i < 6; ++i) {
        const glm::vec4& p = planos[i];

        // "Vertice positivo": la esquina de la caja mas avanzada en la
        // direccion de la normal. Si esa esquina queda detras del plano, TODAS
        // las demas tambien, asi que basta probar una en vez de las ocho.
        glm::vec3 positivo(
            p.x >= 0.0f ? caja.maximo.x : caja.minimo.x,
            p.y >= 0.0f ? caja.maximo.y : caja.minimo.y,
            p.z >= 0.0f ? caja.maximo.z : caja.minimo.z);

        if (p.x * positivo.x + p.y * positivo.y + p.z * positivo.z + p.w < 0.0f)
            return false;
    }
    return true;
}
