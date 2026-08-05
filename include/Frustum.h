#pragma once
#include "AABB.h"

#include <glm/glm.hpp>

// ============================================================================
//  Volumen de vision de la camara, como 6 planos.
//
//  Los planos se extraen directamente de la matriz proyeccion * vista (metodo
//  de Gribb-Hartmann): sumar o restar la cuarta fila a cada una de las tres
//  primeras da los planos izquierdo/derecho, inferior/superior y cercano/lejano
//  ya en coordenadas de mundo. Es preferible a reconstruir las 8 esquinas del
//  frustum porque no hay que invertir ninguna matriz.
//
//  Convenio: un punto esta DENTRO si dot(normal, punto) + d >= 0 en los 6.
// ============================================================================
class Frustum {
public:
    void extraerDe(const glm::mat4& proyeccionVista);

    // false = la caja esta completamente fuera y se puede descartar entera.
    // Puede dar algun falso positivo (caja fuera que se acepta), nunca un falso
    // negativo, que es lo unico que romperia la imagen.
    bool intersecta(const AABB& caja) const;

private:
    glm::vec4 planos[6];   // xyz = normal, w = distancia al origen
};
