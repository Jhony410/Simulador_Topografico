#pragma once
#include <algorithm>
#include <glm/glm.hpp>

// ============================================================================
//  Caja alineada a los ejes (Axis-Aligned Bounding Box).
//  Es el volumen envolvente que usan tanto el Quadtree como el BVH: se elige
//  AABB y no esfera ni caja orientada porque el test contra un plano del
//  frustum se resuelve con una sola comparacion por eje, sin raices ni
//  rotaciones. Es el volumen mas barato de probar de todos.
// ============================================================================
struct AABB {
    glm::vec3 minimo{ 1e30f};
    glm::vec3 maximo{-1e30f};

    void expandir(const glm::vec3& punto) {
        minimo = glm::min(minimo, punto);
        maximo = glm::max(maximo, punto);
    }
    void expandir(const AABB& otra) {
        minimo = glm::min(minimo, otra.minimo);
        maximo = glm::max(maximo, otra.maximo);
    }

    bool valida()  const { return minimo.x <= maximo.x; }
    glm::vec3 centro()     const { return (minimo + maximo) * 0.5f; }
    glm::vec3 extension()  const { return maximo - minimo; }

    // Distancia del punto a la caja (0 si esta dentro). La usa el criterio de
    // nivel de detalle del Quadtree.
    float distanciaA(const glm::vec3& p) const {
        glm::vec3 d = glm::max(glm::max(minimo - p, glm::vec3(0.0f)), p - maximo);
        return glm::length(d);
    }
};
