#pragma once
#include <glm/glm.hpp>
#include <vector>

class Terreno;
class MapaExploracion;

// ============================================================================
//  MODELO: Marching Squares sobre el mapa de alturas.
//  Extrae los segmentos de una isolinea recorriendo SOLO las celdas ya
//  exploradas. Cero llamadas a OpenGL: entra una grilla de numeros y sale una
//  lista de segmentos en coordenadas de mundo (plano XZ).
// ============================================================================
namespace MarchingSquares {

struct Segmento {
    glm::vec2 a{0.0f};   // (x, z) en mundo
    glm::vec2 b{0.0f};
};

// 'paso' submuestrea la grilla: 1 usa todas las celdas, 2 una de cada dos.
// Los segmentos se AÑADEN a 'salida' (no la limpia).
void generarSegmentos(const Terreno& terreno,
                      const MapaExploracion& mapa,
                      float nivel,
                      int paso,
                      std::vector<Segmento>& salida);

} // namespace MarchingSquares
