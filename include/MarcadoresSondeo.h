#pragma once
#include "AABB.h"
#include "BVH.h"

#include <glm/glm.hpp>
#include <vector>

class Terreno;
struct EscalaMundo;

// ============================================================================
//  MODELO: estacas topograficas sembradas por el terreno.
//
//  Cada marcador es una torreta delgada apoyada SIEMPRE sobre la superficie
//  real: su base se muestrea del heightmap en su propio (x,z), de modo que en
//  una ladera la estaca nace en la pendiente y no flotando en el aire.
//
//  La altura sale de EscalaMundo (una fraccion muy pequena de la diagonal del
//  terreno), nunca de una constante global: en un mapa grande las torretas
//  siguen siendo torretas y no rascacielos.
//
//  Las posiciones se generan con un PRNG de SEMILLA FIJA: son pseudoaleatorias
//  pero deterministas, asi que el mismo mapa siempre siembra los mismos
//  marcadores sin necesidad de guardarlos en disco.
// ============================================================================
struct MarcadorSondeo {
    glm::vec3 base{0.0f};     // apoyo sobre el relieve (y = alturaEn(x, z))
    float     altura = 0.0f;  // largo del mastil
    float     ancho  = 0.0f;  // semiancho de la base y de las crucetas
};

class MarcadoresSondeo {
public:
    void generar(const Terreno& terreno, const EscalaMundo& escala,
                 int cantidad, unsigned int semilla);
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
