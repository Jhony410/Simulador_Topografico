#pragma once
#include "AABB.h"
#include "BVH.h"

#include <glm/glm.hpp>
#include <vector>

class Terreno;

// ============================================================================
//  MODELO: estacas topograficas sembradas por el terreno.
//  Cada marcador es un poste vertical con un cuadrito en la punta. Unos se
//  apoyan en el relieve y otros quedan flotando, igual que en la referencia.
//
//  Las posiciones se generan con un PRNG de SEMILLA FIJA: son pseudoaleatorias
//  pero deterministas, asi que el mismo mapa siempre siembra los mismos
//  marcadores sin necesidad de guardarlos en disco ni de que el jurado vea una
//  escena distinta en cada arranque.
// ============================================================================
struct MarcadorSondeo {
    glm::vec3 base{0.0f};   // extremo inferior del poste
    float     altura = 0.0f;  // largo del segmento vertical
};

class MarcadoresSondeo {
public:
    void generar(const Terreno& terreno, int cantidad, unsigned int semilla);
    void limpiar();

    const std::vector<MarcadorSondeo>& obtener() const { return marcadores; }
    bool  vacio()    const { return marcadores.empty(); }
    std::size_t cantidad() const { return marcadores.size(); }

    // Jerarquia de cajas envolventes para descartar por frustum.
    const BVH& obtenerBVH() const { return bvh; }

private:
    std::vector<MarcadorSondeo> marcadores;
    std::vector<AABB>           cajas;
    BVH                         bvh;
};
