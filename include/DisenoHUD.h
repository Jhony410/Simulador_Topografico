#pragma once
#include <glm/glm.hpp>

// ============================================================================
//  Geometria (layout) del HUD en pixeles: pura matematica, sin OpenGL.
//  Vive aparte porque la necesitan DOS capas: la Vista para dibujar y el
//  Controlador para saber si un clic cayo dentro de una zona interactiva. Si
//  estuviera dentro de VistaHUD, el Controlador acabaria dependiendo de la GPU.
//
//  Convencion: (x, y) es la esquina superior izquierda; el eje Y crece hacia
//  abajo, igual que la proyeccion ortografica del HUD.
//  Devuelve vec4(x, y, ancho, alto).
//
//  El HUD en juego es MINIMO: marca arriba a la izquierda, minimapa con su
//  porcentaje abajo a la izquierda y lista de controles abajo a la derecha.
//  Todo lo tecnico vive en el panel de depuracion (F3) y no ocupa layout fijo.
// ============================================================================
namespace DisenoHUD {

// Carril fino de progreso, justo debajo del minimapa.
glm::vec4 rectBarraProgreso(float anchoPantalla, float altoPantalla);

// Bloque de texto de controles, abajo a la derecha. 'lineas' fija su alto.
glm::vec4 rectControles(float anchoPantalla, float altoPantalla, int lineas);

// Panel de depuracion (F3), arriba a la derecha.
glm::vec4 rectPanelDebug(float anchoPantalla, float altoPantalla, int lineas);

// Prueba de impacto de un punto contra un rectangulo.
bool puntoDentro(double px, double py, const glm::vec4& rect);

} // namespace DisenoHUD
