#pragma once
#include <glm/glm.hpp>

// ============================================================================
//  Geometria (layout) del HUD en pixeles: pura matematica, sin OpenGL.
//  Vive aparte porque la necesitan DOS capas: la Vista para dibujar los
//  botones y el Controlador para saber si un clic cayo dentro de uno. Si
//  estuviera dentro de VistaHUD, el Controlador acabaria dependiendo de la GPU.
//
//  Convencion: (x, y) es la esquina superior izquierda; el eje Y crece hacia
//  abajo, igual que la proyeccion ortografica del HUD.
//  Devuelve vec4(x, y, ancho, alto).
// ============================================================================
namespace DisenoHUD {

glm::vec4 rectBotonMapa(int indice, float anchoPantalla, float altoPantalla);

// 0 = arriba, 1 = izquierda, 2 = abajo, 3 = derecha, 4 = SHIFT, 5 = SPACE
glm::vec4 rectTecla(int indice, float anchoPantalla, float altoPantalla);

// Carril completo de la barra de progreso (abajo a la izquierda). El tramo
// amarillo es una fraccion de este ancho.
glm::vec4 rectBarraProgreso(float anchoPantalla, float altoPantalla);

// Recuadro del panel holografico de curvas, justo encima de la barra.
glm::vec4 rectPanelCurvas(float anchoPantalla, float altoPantalla);

// Panel oscuro del texto de mision (arriba, centrado).
glm::vec4 rectPanelMision(float anchoPantalla, float altoPantalla);

// Panel informativo que entra por la derecha al completar la mision.
glm::vec4 rectPanelLateral(float anchoPantalla, float altoPantalla);

// Circulo amarillo con la X, arriba a la derecha. Es clicable: el Controlador
// lo consulta para cerrar la aplicacion.
glm::vec4 rectBotonSalir(float anchoPantalla, float altoPantalla);

// Icono de menu (tres lineas) debajo del boton de salida. Decorativo.
glm::vec4 rectIconoMenu(float anchoPantalla, float altoPantalla);

// Prueba de impacto de un punto contra un rectangulo.
bool puntoDentro(double px, double py, const glm::vec4& rect);

} // namespace DisenoHUD
