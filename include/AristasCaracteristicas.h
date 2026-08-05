#pragma once
#include <vector>

// ============================================================================
//  MODELO: extraccion de aristas caracteristicas de una malla.
//
//  EL PROBLEMA QUE RESUELVE
//  ------------------------
//  Dibujar un modelo "en alambre" con glPolygonMode(GL_LINE) pinta las TRES
//  aristas de cada triangulo, diagonales incluidas. Con 83.712 triangulos, a
//  tamaño de pantalla eso son mas lineas que pixeles: el dron se lee como una
//  mancha solida, no como un armazon. La referencia de Orano usa un modelo
//  low-poly donde cada linea SI se distingue.
//
//  LA SOLUCION
//  -----------
//  Quedarse solo con las aristas que un dibujante trazaria:
//    - de PLIEGUE: las que separan dos caras cuyo angulo entre normales supera
//      un umbral (los cantos reales del objeto);
//    - de BORDE: las que pertenecen a una sola cara (bordes abiertos de la malla).
//  Las aristas interiores de una superficie lisa se descartan, que son
//  justamente las que emborronaban la silueta.
//
//  Antes hay que SOLDAR los vertices por posicion: los exportadores duplican
//  vertices para partir normales o coordenadas de textura, y sin soldar cada
//  cara creeria tener aristas propias y ninguna saldria compartida.
// ============================================================================
namespace AristasCaracteristicas {

// 'vertices' viene intercalado (floatsPorVertice por vertice, los 3 primeros
// son la posicion). Devuelve pares de indices listos para GL_LINES, referidos
// al arreglo de vertices ORIGINAL.
std::vector<unsigned int> extraer(const std::vector<float>& vertices,
                                  int floatsPorVertice,
                                  const std::vector<unsigned int>& indices,
                                  float anguloGrados,
                                  float epsilonSoldadura);

} // namespace AristasCaracteristicas
