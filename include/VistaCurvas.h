#pragma once
#include "CurvasNivel.h"
#include "Limites.h"

#include <glad/glad.h>
#include <glm/glm.hpp>

class GestorRecursos;

// ============================================================================
//  VISTA: panel holografico de curvas de nivel (esquina inferior izquierda).
//
//  El panel es una VISTA APARTE, no un elemento 2D del HUD: se recorta el
//  viewport a su rectangulo y se dibuja el plano con su propia camara en
//  perspectiva. Asi el escorzo lo produce la proyeccion real y no un truco de
//  dibujo, que es lo que le da el aspecto de holograma inclinado.
// ============================================================================
class VistaCurvas {
public:
    void inicializar(GestorRecursos& recursos);
    void liberar();

    // Reconstruye el VBO a partir de las polilineas del Modelo. 'limites' sirve
    // para normalizar el mundo al cuadrado [-1, 1] del plano.
    void actualizarCurvas(const CurvasNivel& curvas, const LimitesMundo& limites);

    // Rectangulo del panel en pixeles. Vive aqui, y ya no en DisenoHUD, porque
    // esta vista es su unica consumidora: el HUD en vuelo no lo muestra.
    static glm::vec4 rectangulo(float anchoPantalla, float altoPantalla);

    // Devuelve el numero de draw calls emitidas.
    int dibujar(int anchoPantalla, int altoPantalla);

private:
    GLuint programa = 0;
    GLint  locModelo = -1, locVista = -1, locProyeccion = -1, locAlpha = -1;

    GLuint vao = 0, vbo = 0;
    int    verticesCurvas = 0;   // rango [0, verticesCurvas)
    int    verticesBorde  = 0;   // rango siguiente: marco del plano
};
